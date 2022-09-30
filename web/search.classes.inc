<?php

@include_once('./search.config.inc');


$dirInc = './';

$styleAry = array();
if ( @include_once($dirInc.'citeproc.styles.tools.inc') /* for websearch_citeproc_style_list_short() */ ) {
	if ( @include_once($dirInc.'meta-remap.tools.inc') ) {
		// following is a bit of a mess to get $styleAry, but finally we won't have it selectable!(?)
		$cslTmpAry = $cslAry;
		$styleAllAry = websearch_citeproc_style_list();
		$styleAry = ( @empty($cslAry) ? $styleAllAry : array() );
		foreach( @$cslTmpAry as $fIdx => $fVal ) {
			foreach( $styleAllAry as $sIdx => $sAry ) {
				if ( $fVal != $sAry['file'] ) { continue; }
				$styleAry[$sIdx] = $sAry;
				unset($cslTmpAry[$fVal]);
				break;
			}
		}
	}
}

class apiQueryUtils
{
	public $apiName;		// e.g. "dora", required (this is basically the work-name for us)
	public $apiHost;		// optional, apiName is taken if empty, could e.g. be 'libri' (for API name "dora")
	public $apiLabel;		// e.g. "DORA" or "DORA/Eawag", required
	public $apiScope;		// e.g. "Eawag" or catalog name, optional
	public $apiFacet;		// API-specific filter term, optional
	public $apiKey;
	public $apiLanguage;
	public $apiHeaderData;		/* and array with further header parts/'quotations'	*/
	public $apiUrl = '';		/* see this->setApiUrl() for debug hint */
	public $webUrl = '';
	
	public $searchTermOrig;
	public $searchTerm;
	public $searchLimit;	// numerical
	public $searchOffset;	// numerical
	public $citeStyle = 'national-institute-of-health-research';
	public $formName;
	public $formTarget;

	public function __construct($apiLabel = '')
	{
		if ( !empty($apiLabel) ) {
			$ary = array_map('trim',explode('/',$apiLabel,2));		// for safety, should we get e.g. DORA/Eawag
			if ( empty($this->apiName) ) {
				$this->apiName = strtolower($ary[0]);
			}
			if ( empty($this->apiScope) && !empty($ary[1]) ) {
				$this->apiScope = $ary[1];
			}
		}
		if ( !is_array($this->apiHeaderData) ) {
			$this->apiHeaderData = ( empty($this->apiHeaderData) ? array() : array($this->apiHeaderData) );
		}
	}

	public function cleanTerm($searchTerm = '')
	{
		// clean-up for/against quotes+tags:
		return implode(' ',array_map('trim',str_getcsv(preg_replace('/\s+/',' ',strip_tags(urldecode($searchTerm))),' ')));
	}

	public function cleanIssn($issn = '',$tuneFunc = 'strtoupper')
	{
		$issn = str_ireplace( array('&minus;','&#8222;','&mdash;','&#8212;','&ndash;','&#8211;'), array('-','-','-','-','-','-'), strip_tags($issn) );
		if ( $issn = trim(urldecode($issn)) ) {
			$issn = preg_replace('/\-{2,}/','-',preg_replace('/\s+/','',$issn));
		}
		if ( @!empty($tuneFunc) && function_exists($tuneFunc) ) {
			$issn = $tuneFunc($issn);
		}
		return $issn;
	}

	public function getSearchTerm($searchTerm = '')
	{
		$searchTerm = $this->cleanTerm($searchTerm);
		if ( empty($searchTerm) ) {
			$searchTerm = $this->cleanTerm($this->searchTerm);
			if ( empty($searchTerm) ) {
				$searchTerm = $this->cleanTerm($_GET['find']);
			}
		}
		if ( @intval($searchTerm) && $this->searchTermOrig != $searchTerm ) { // let's see if this is an ISSN
			$ary = array();
			preg_match('/\d{4}\s*\-*\s*\d{4}|\d{4}\s*\-*\s*\d{3}X/',$searchTerm,$ary);
			if ( @intval($ary[0]) ) {
				$searchTerm = $this->cleanIssn($searchTerm);
			}
		}
		if ( !empty($searchTerm) && $this->searchTermOrig != $searchTerm ) {
			$this->searchTermOrig = $searchTerm;
			$this->searchTerm = $searchTerm;
		}
		return $searchTerm;
	}

	public function exitByError($eMsg = 'ERROR') {
		header('Content-Type: ' . 'application/json' . ' charset=utf-8');
		print json_encode( array('error' => rtrim($eMsg)), JSON_PRETTY_PRINT );
		exit;
	}

	public function jsonOutput($jsonData, $dataType = null ) {

		if ( empty($dataType) || $dataType == 'file' ) {
			header('Content-Type: ' . 'application/json' . ' charset=utf-8');
			print ( is_string($jsonData) ? $jsonData : json_encode( $jsonData, JSON_PRETTY_PRINT ) );
			exit;
		}

		if ( $dataType == 'string' ) {		// output as condensed/unchanged string
			return ( is_string($jsonData) ? $jsonData : json_encode( $jsonData) );
		}
		if ( $dataType == 'pretty' ) {		// output as pretty string
			return json_encode( ( is_string($jsonData) ? json_decode($jsonData) : $jsonData ), JSON_PRETTY_PRINT );
		}

		if ( $dataType == 'array' ) {		// output as array
			return ( is_string($jsonData) ? json_decode($jsonData,true) : ( is_array($jsonData) ? $jsonData : json_decode(json_encode($jsonData),true) ) );
		}
		if ( $dataType == 'object' ) {		// output as object
			return ( is_string($jsonData) ? json_decode($jsonData,false) : ( is_array($jsonData) ? json_decode(json_encode($jsonData),false) : $jsonData ) );
		}

		return $jsonData;
	}

	public function setApiUrl( $apiUrl = '' ) {
		$this->apiUrl = $apiUrl;
		if ( @intval($_GET['dev-test']) == 3 && websearch_ip_from('dev') ) { echo print_r( $this->apiUrl, true ); exit; }
		return $this->apiUrl;
	}

	public function setWebUrl( $webUrl = '' ) {
		$this->webUrl = $webUrl;
		if ( @intval($_GET['dev-test']) == 4 && websearch_ip_from('dev') ) { echo print_r( $this->webUrl, true ); exit; }
		return $this->webUrl;
	}

	public function jsonDecode( $jsonCode = '', $returnAsArray = false ) {
		$jsonItem = json_decode( $jsonCode, $returnAsArray );
		if ( @intval($_GET['dev-test']) == 5 && websearch_ip_from('dev') ) { echo print_r( $jsonItem, true ); exit; }
		return $jsonItem;
	}

	public function setupParam($propName = '')
	{
		$limitAry = array( 
			'default' => 10,
			'journal' => 20,
		);
		$argAry = array(
		/*	'find'   => @trim(rawurldecode($_GET['find'])),		*/
			'limit'  => ( @isset($_GET['limit']) ? min(max(intval($_GET['limit']),0),500) : -1 ),
			'start'  => @intval($_GET['start']),	/* send by the form, legacy support */
			'offset' => @intval($_GET['offset']),
			'style'  => @trim($_GET['style']),
			'lang'   => @trim($_GET['lang']),
			'scope'  => @trim($_GET['scope']),		/* for Lib4RI/DORA basically, which institute/namepace to search */
			'facet'  => @trim($_GET['facet']),		/* for SLSP/swisscovery */
		);
		$argStr = '';	// http_build_query($argAry) worked strange...!(?)
		foreach( $argAry as $idx => $arg ) {
			$arg = $this->cleanTerm($arg);		// also for safety reasons!
			$argAry[$idx] = $arg;
			if ( !empty($idx) && $arg !== '' ) { $argStr .= ( empty($argStr) ? '?' : '&' ) . $idx . '=' . $arg; }
		}
		$this->searchTerm = $this->getSearchTerm();
		$this->searchLimit = ( $argAry['limit'] < 0 ? ( @isset($limitAry[$this->apiName]) ? $limitAry[$this->apiName] : $limitAry['default'] ) : $argAry['limit'] );
		$this->searchOffset = max( $argAry['start'], $argAry['offset'] );
		if ( @empty($this->apiScope) && !empty($argAry['scope']) ) {
			$this->apiScope = $argAry['scope'];
		}
		if ( @empty($this->apiFacet) && !empty($argAry['facet']) ) {
			$this->apiFacet = $argAry['facet'];
		}
		$this->formName = rtrim( 'lib4ri-websearch-' . @strval($this->apiName) , '-._' ) . ( empty($this->apiScope) ? '' : '-'.strtolower($this->apiScope) );
		$this->formTarget = $argStr;
		if ( !empty($argAry['style']) ) {
			$this->citeStyle = strtolower($argAry['style']);
		}
		$this->apiLanguage = ( @empty($argAry['lang']) ? 'en-US' : strtolower(substr($argAry['lang'],0,2)) . @strtoupper(substr($argAry['lang'],2)) );
		
		return ( ( empty($propName) || @!isset($this->$propName) ) ? TRUE /* basically all done here */ : $this->$propName );
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)	/* dummy/placehoder function, however keep! */
	{
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		return ( 'https://api.example.com/?q=' . $searchTermEnc );
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)	/* dummy/placehoder function, however keep! */
	{
		// API-corresponding web link. The human user must get the same results as we got via API!
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		return ( 'https://www.example.com/?q=' . $searchTermEnc );
	}

	public function makeApiQueryHeader($method = '', $userAgent = '')
	{
		$httpData = array(		/* just to pretend human browser behavior */
			'http' => array(
				'header' => array(
					'User-Agent: ' . ( empty($userAgent) ? 'Mozilla/5.0 (Lib4RI IT Services)' : $userAgent ),
					'Accept-Language: ' . 'en-us,en;q=0.5',
					'Connection: close',
				),
				'method' => ( empty($method) ? 'GET' : $method ),
			),
		);
		foreach( $this->apiHeaderData as $key => $val ) {
			$httpData['http']['header'][] = $key . ': '. $val;
		}
		return $httpData;
	}

	public function makeQuery($apiUrl, $timeout = 60, $userAgent = '')
	{
		if ( @intval($_GET['dev-test']) == 1 && websearch_ip_from('dev') ) { echo print_r( $apiUrl, true ); exit; }

		if ( $timeout > 0 ) {		// with cURL:
			$headerAry = $this->makeApiQueryHeader('',$userAgent);
			$cUrl = curl_init();
			curl_setopt($cUrl, CURLOPT_URL, $apiUrl );
			curl_setopt($cUrl, CURLOPT_HTTPHEADER, $headerAry['http']['header'] );
			curl_setopt($cUrl, CURLOPT_RETURNTRANSFER, 1 );
			curl_setopt($cUrl, CURLOPT_TIMEOUT, $timeout );
			curl_setopt($cUrl, CURLOPT_CONNECTTIMEOUT, $timeout );
			$jsonCode = curl_exec($cUrl);
			curl_close($cUrl);
			/*
			if ( @is_array($http_response_header) && sizeof($http_response_header) ) {
				$resp0 = preg_replace('/\s+/','_',trim($http_response_header[0]));
				if ( stripos($resp0,'HTTP') === false || ( !strpos($resp0.'_','_200_') && !strpos($resp0.'_','_OK_') ) ) {
					// an error happened. Let's merge the real response with the http response:
					return json_encode( array(
							'query_status' => 'error',
							'query_header' => $this->makeApiQueryHeader(),
							'query_url' => $apiUrl,
							'json_data_from_api' => json_decode($jsonCode),
							'http_response_header' => $http_response_header,
						),
						JSON_PRETTY_PRINT );
				}
			}
			*/
			return $jsonCode;
		}
		// with PHP own functions:
		$jsonCode = @file_get_contents($apiUrl,false,stream_context_create($this->makeApiQueryHeader()));
		if ( @is_array($http_response_header) && sizeof($http_response_header) ) {
			$resp0 = preg_replace('/\s+/','_',trim($http_response_header[0]));
			if ( stripos($resp0,'HTTP') === false || ( !strpos($resp0.'_','_200_') && !strpos($resp0.'_','_OK_') ) ) {
				// an error happened. Let's merge the real response with the http response:
				/* $http_response_header : Array (
					[0] => HTTP/1.1 200 OK
					[1] => p3p: CP="IDC DSP COR ADM DEVi TAIi PSA PSD IVAi IVDi CONi HIS OUR IND CNT"
					[2] => X-Request-ID: drRTg4WVSi
					[3] => vary: accept-encoding
					[4] => X-Exl-Api-Remaining: 29963
					[5] => Access-Control-Allow-Origin: *
					[6] => Access-Control-Allow-Methods: GET,POST,DELETE,PUT,OPTIONS
					[7] => Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept, Authorization
					[8] => Content-Type: application/json;charset=UTF-8
					[9] => Date: Wed, 16 Mar 2022 13:05:08 GMT
					[10] => Connection: close
					[11] => Server: CA-API-Gateway/9.0
				)	*/
				return json_encode( array(
						'query_status' => 'error',
						'query_header' => $this->makeApiQueryHeader(),
						'query_url' => $apiUrl,
						'json_data_from_api' => json_decode($jsonCode),
						'http_response_header' => $http_response_header,
					),
					JSON_PRETTY_PRINT );
			}
		}
		return $jsonCode;
	}

	public function queryNow($searchTerm = '')
	{
		if ( empty($searchTerm) ) {
			$this->setupParam();		// refresh!
			$searchTerm = ( empty($this->searchTermOrig) ? 'rainbow' : $this->searchTermOrig );
		}
		if ( strlen(urldecode($searchTerm)) < 2 && urldecode($searchTerm) != '*' ) {
			$searchTerm = 'nanosphere';
		}
		$apiUrl = $this->apiUrl( empty($searchTerm) ? 'dreamland' : $searchTerm );
		return $this->makeQuery( $apiUrl );
	}

}

// ==================================================================================

class apiQueryLib4ri extends apiQueryUtils
{
	public function __construct($apiLabel)
	{
		parent::__construct($apiLabel);
		$this->setupParam();
	}

	public function makeForm( $styleAry = array() )
	{
		$htmlAry[] = "<form action='{$this->formTarget}' name='{$this->formName}-form' id='{$this->formName}-form' class='{$this->formName}-form'><br>";
		$htmlAry[] = "<b>{$this->apiLabel}</b>: &nbsp; <input type='text' name='find' id='lib4ri-websearch-{$this->apiName}-value' value='{$this->searchTermOrig}' style='border:0px; width:24em; background-color:#bce'>&nbsp;&nbsp;";
		if( @!empty($styleAry) ) {	// aux selection setup
			$htmlAry[] = "<br><br><select name='style' id='{$this->formName}-style' style='border:0px; width:33em; background-color:#bce;'>";
			foreach( $styleAry as $sAry ) {
				$tmp = ( empty($sAry['short']) ? '' : $sAry['short'].' - ' ) . str_replace("\\",'',$sAry['full']);
				$htmlAry[] = "<option value=\"" . $sAry['file'] . "\"" . ( $sAry['file'] == $this->citeStyle ? ' selected' : '' ) . ">" . $tmp . "</option>\r\n";
			}
			$htmlAry[] = "</select>&nbsp;&nbsp;";
		} else {	// as intended (keeping a form field currently):
			$htmlAry[] = '<input type="hidden" name="style" value="' . $this->citeStyle . '">';
		}
		$htmlAry[] = "<input type='submit' value='find' style='border:2px;'></form>";
		return implode('',$htmlAry);
	}

	public function showForm( $styleAry = array() )
	{
		echo $this->makeForm( $styleAry );
		if ( empty($this->searchTermOrig) /* to be tuned/replaced */ ) { die('<hr></body><html>'); }
	}
}

// -----------------------------------------------------------------------------------

class apiQueryWoS extends apiQueryLib4ri
{
	// see https://developer.clarivate.com/apis/woslite		<== for details click on 'GET' buttons

	/* see also Clarivate support e-mail (web-link related) from 2021-09-08 about Field Tags:
		- TS=Topic
		- TI=Title
		- AB=Abstract
		- AU=Author
		- AI=Author Identifiers
		- AK=Author Keywords
		- GP=Group Author
		- ED=Editor
		- KP=Keyword Plus
		- SO=Publication Titles
		- DO=DOI
		- PY=Year Published
		- CF=Conference
		- AD=Address
		- OG=Affiliation
		- OO=Organization
		- SG=Suborganization
		- SA=Street Address
		- CI=City
		- PS=Province/State
		- CU=Country/Region
		- ZP=Zip/Postal Code
		- FO=Funding Agency
		- FG=Grant Number
		- FD=Funding Details
		- FT=Funding Text
		- SU=Research Area
		- WC=Web of Science Categories
		- IS= ISSN/ISBN
		- UT=Accession Number
		- PMID=PubMed ID
		- DOP=Publication Date
		- PUBL=Publisher
		- ALL=All Fields
		- FPY=Final publication year
	*/

	public function __construct()
	{
		$this->apiLabel = 'WoS';
		$this->apiKey = @strval( W_S_API_WOS_KEY );
		$this->apiHeaderData = array( 'X-ApiKey' => $this->apiKey );
		parent::__construct($this->apiLabel);
	}
	
	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset ) + 1;		// +1 since with WoS we need to indicate the first record
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		$apiUrl = str_replace('###',$searchTermEnc,'https://wos-api.clarivate.com/api/woslite/?usrQuery=ALL%3d(###)');
		return ( $apiUrl . '&databaseId=WOS&count=' . $this->searchLimit . '&firstRecord=' . $searchOffset );
		// for example (with cURL):
		// curl -X GET "https://wos-api.clarivate.com/api/woslite/?databaseId=WOS&usrQuery=TI%3d(hydro)%20OR%20AB%3d(hydro)%20OR%20AU%3d(hydro)&count=10&firstRecord=1" -H "accept: application/json" -H "X-ApiKey: a123....789f"
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		// $searchOffset = ... 	<== offset handling unknown for web links
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		return str_replace('###',$searchTermEnc,'https://www.webofscience.com/wos/woscc/general-summary?query=ALL%3d(###)');
		// for example:
		// https://www.webofscience.com/wos/woscc/general-summary?query=TI%3d(hydro)%20OR%20AB%3d(hydro)%20OR%20AU%3d(hydro)
	}
}

class apiQueryScopus extends apiQueryLib4ri
{
	// see https://dev.elsevier.com/documentation/ScopusSearchAPI.wadl
	// see https://dev.elsevier.com/sc_search_tips.html

	public function __construct()
	{
		$this->apiLabel = 'Scopus';
		$this->apiKey = @strval( W_S_API_SCOPUS_KEY );
		$this->apiHeaderData = array( 'X-ELS-APIKey' => $this->apiKey );
		parent::__construct($this->apiLabel);
	}
	
	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		$apiUrl = 'https://api.elsevier.com/content/search/scopus?query=ALL(' . $searchTermEnc . ')';
		// $apiUrl .= '&APIKey=' . $this->apiKey; // would work, but not required, since set up via header currently (safer?)
		return ( $apiUrl . '&count=' . $this->searchLimit . '&start=' . $searchOffset . '&httpAccept=application/json' );
		// for example:
		// https://api.elsevier.com/content/search/scopus?query=TITLE-ABS-KEY-AUTH(filtration)&count=15&APIKey=fedc....4321&httpAccept=application/json
		// https://api.elsevier.com/content/search/scopus?query=ALL(filtration)&count=15&APIKey=fedc....4321&httpAccept=application/json
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset ) + 1;	// +1, see/test example link!
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		$webUrl = 'https://www.scopus.com/results/results.uri?src=s&st1=' . $searchTermEnc . '&sot=b&s=ALL(' . $searchTermEnc . ')';
		return( $webUrl . '&cc=' . $this->searchLimit . '&offset=' . $searchOffset . '&sid=new' );
		// for example:
		// https://www.scopus.com/results/results.uri?src=s&searchterm1=Filtration&sot=b&s=TITLE-ABS-KEY-AUTH(Filtration)&sid=new&cc=50&offset=51
		// https://www.scopus.com/results/results.uri?src=s&searchterm1=Filtration&sot=b&s=TITLE-ABS-KEY-AUTH(Filtration)&sid=new&cc=50&offset=51
	}
}

class apiQuerySLSP extends apiQueryLib4ri
{
	// see https://developers.exlibrisgroup.com/blog/understanding-and-using-the-primo-search-api-as-a-guest-in-the-developer-network/

	public $apiVid;
	public $apiCatAry;

	public function __construct($apiSearchScope = '')
	{
		$this->apiName = 'slsp';		// lower-case!
		$this->apiLabel = 'swisscovery';
		$this->apiKey = @strval( W_S_API_SLSP_KEY );

		if ( stripos('/union/',$apiSearchScope) ) {	// special case for non-lib4ri search across all libraries
			$this->apiCatAry = array(
				'union' => array(
					'scope' => 'DiscoveryNetwork',
					'tab' =>  'DiscoveryNetwork',
					'label' => 'All Libraries',
					'rtype' => 'exclude:journals;articles;newspaper_articles',
				),
			);
			$this->apiVid = '41SLSP_NETWORK:VU1_UNION';
		} else {
			$this->apiVid = '41SLSP_LIB:GO';

			$this->apiCatAry = array(	 /* Catalogues, with corresponding search keys/parameters: */
				'myinstitution' => array(
					'scope' => 'MyInstitution',
					'tab' =>  'LibraryCatalog' /* '41SLSP_LIB_DN_CI'	*/,
					'label' => 'Lib4RI',
					'rtype' => 'exclude:journals;articles;newspaper_articles',	/* for media types, if this scpoe does not cover CI, exluding articles,newspaper_articles is not required */
				),
				'centralindex' => array(
					'scope' => 'CentralIndex',
					'tab' => 'CDI',
					'label' => 'Central Discovery Index (CDI)',
					'rtype' => 'exclude:articles;newspaper_articles',		/* for media types */
				),
				'discoverynetwork' => array(
					'scope' => 'DiscoveryNetwork',
					'tab' => 'DN',
					'label' => 'swisscovery without CDI',
					'rtype' => 'exclude:articles;newspaper_articles',		/* for media types */
				),
				'dn_and_ci' => array(
					'scope' => 'DN_and_CI',
					'tab' => '41SLSP_LIB_DN_CI',
					'label' => 'swisscovery',
					'rtype' => 'exclude:books;book_chapters|include:journals',			/* for media types */
					'facet' => 'include:online_resources',
				),
			);
		}
		if ( !( $cat = @trim(strip_tags($_GET['catalog'])) ) ) {
			if ( !( $cat = @trim($apiSearchScope) ) ) {
				$cat = 'DiscoveryNetwork';
			}
		}
		$this->apiScope = $cat;

		parent::__construct($this->apiLabel);
	}

	public function getQueryField($searchTerm = '', $qField = 'any')
	{
		if ( $searchTerm = @strtoupper(trim(urldecode($searchTerm))) ) {
			$ary = array();
			preg_match('/\d{4}\s*\-*\s*\d{4}|\d{4}\s*\-*\s*\d{3}X/',$searchTerm,$ary);
			return ( @intval($ary[0]) ? 'issn' : $qField );
		}
		return $qField;
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$apiUrlAry = array(
			'q'      => $this->getQueryField($searchTerm) . ',contains,' . urldecode( empty($searchTerm) ? $this->searchTermOrig : $searchTerm ),
			'offset' => $searchOffset,
			'limit'  => $this->searchLimit,
			'lang'   => substr($this->apiLanguage,0,2),
			'sort'   => 'rank',
			'getMore'        => 0,
			'pcAvailability' => 'true',
			'conVoc'         => 'true',
			'skipDelivery'   => 'true',
			'disableSplitFacets' => 'true',
			'vid'    => $this->apiVid,
			'inst'   => strtok($this->apiVid.':',':'),
			'tab'    => $catAry['tab'],
			'scope'  => $catAry['scope'],
			'apikey' => $this->apiKey,
		);
		$apiUrl = 'https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?' . http_build_query($apiUrlAry,'','&',PHP_QUERY_RFC3986);
		if ( @!empty($catAry['rtype']) ) {
			foreach( explode('|',$catAry['rtype']) as $tmp ) {
				$cmd = 'qInclude';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'qExclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				$apiUrl .= '&' . $cmd . '=facet_rtype%2Cexact%2C' . implode('%7C%2C%7Cfacet_rtype%2Cexact%2C',$ary);
			}
			// note that 'multiFacets=facet_rtype,...' would work too
		}
		if ( @!empty($catAry['facet']) ) {
			foreach( explode('|',$catAry['facet']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				$apiUrl .= '&multiFacets=facet_tlevel%2C' . $cmd . '%2C' . implode('%7C%2C%7Cfacet_tlevel%2C' . $cmd . '%2C',$ary);
			}
		}
		return $apiUrl;
		// for example:
		// https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?vid=41SLSP_LIB%3AGO&tab=CDI&scope=CentralIndex&q=any%2Ccontains%2Crainbow&lang=eng&offset=0&limit=10&sort=rank&pcAvailability=true&getMore=0&conVoc=true&inst=41SLSP_LIB&skipDelivery=true&disableSplitFacets=true&apikey=7777....ffff
		// or general/union
		// https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=any%2Ccontains%2CWater%20Research&offset=0&limit=10&lang=en&sort=rank&getMore=0&pcAvailability=true&conVoc=true&skipDelivery=true&disableSplitFacets=true&vid=41SLSP_NETWORK:VU1_UNION&inst=41SLSP_NETWORK&tab=DiscoveryNetwork&scope=DiscoveryNetwork&apikey=7777....ffff
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		// Corresponding to the intended API request, also the human user must have/get a corresponding link to the online search page:
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$webUrlAry = array(
			'query'  => $this->getQueryField($searchTerm) . ',contains,' . urldecode($searchTerm),
			'offset' => $searchOffset,
			'limit'  => $this->searchLimit,
			'lang'   => substr($this->apiLanguage,0,2),
			'vid'    => $this->apiVid,
			'tab'    => $catAry['tab'],
			'search_scope'  => $catAry['scope'],
		);
		$webUrl = 'https://' . ( stripos('/union/',$this->apiScope) || stripos('/union/',$catAry['scope']) ? '' : 'lib4ri.' );
		$webUrl .= 'swisscovery.slsp.ch/discovery/search?' . http_build_query($webUrlAry,'','&',PHP_QUERY_RFC3986);
		if ( @!empty($catAry['rtype']) ) {
			foreach( explode('|',$catAry['rtype']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				foreach( $ary as $i => $tmp /* name of resource type */ ) {
					$webUrl .= '&mfacet=rtype,' . $cmd . ',' . $tmp . ',' . strval($i+1);
				}
			}
		}
		if ( @!empty($catAry['facet']) ) {
			foreach( explode('|',$catAry['facet']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				foreach( $ary as $i => $tmp /* name of resource type */ ) {
					$webUrl .= '&mfacet=tlevel,' . $cmd . ',' . $tmp . ',' . strval($i+1);
				}
			}
		}
		return $webUrl;
		// for example:
		// https://lib4ri.swisscovery.slsp.ch/discovery/search?query=any,contains,rainbow&tab=CDI&search_scope=CentralIndex&vid=41SLSP_LIB:GO&lang=en&offset=0
		// or general/union
		// https://swisscovery.slsp.ch/discovery/search?query=any,contains,Water%20Research&tab=DiscoveryNetwork&search_scope=DiscoveryNetwork&vid=41SLSP_NETWORK:VU1_UNION&offset=0
	}
}

class apiQueryBook extends apiQueryLib4ri
{
	// see https://developers.exlibrisgroup.com/blog/understanding-and-using-the-primo-search-api-as-a-guest-in-the-developer-network/

	public $apiVid;
	public $apiCatAry;

	public function __construct($apiSearchScope = '')
	{
		$this->apiName = 'book';		// lower-case!
		$this->apiHost = 'slsp';
		$this->apiLabel = 'swisscovery';
		$this->apiKey = @strval( W_S_API_SLSP_KEY );
		$this->apiVid = '41SLSP_LIB:GO';
		if ( stripos('/national/',$apiSearchScope) ) {	// special case for non-lib4ri search across all libraries
			$this->apiCatAry = array(
				'national' => array(
					'scope' => 'DN_and_CI',
					'tab' =>  '41SLSP_LIB_DN_CI',
					'label' => 'All Libraries',
					'rtype' => 'exclude:journals;articles;newspaper_articles',
					'exclude:articles;journals;workingpaper;newsletterarticle;reviews;standards;newspaper_articles',
				),
			);
		} else {
			$this->apiCatAry = array(	 /* Catalogues, with corresponding search keys/parameters: */
				'myinstitution' => array(
					'scope' => 'MyInstitution',
					'tab' =>  'LibraryCatalog' /* '41SLSP_LIB_DN_CI'	*/,
					'label' => 'Lib4RI',
					'rtype' => 'exclude:articles;journals;workingpaper;newsletterarticle;reviews;standards;newspaper_articles',
				),
				'centralindex' => array(
					'scope' => 'CentralIndex',
					'tab' => 'CDI',
					'label' => 'Central Discovery Index (CDI)',
					'rtype' => 'exclude:journals;articles;newspaper_articles',		/* for media types */
				),
				'discoverynetwork' => array(
					'scope' => 'DiscoveryNetwork',
					'tab' => 'DN',
					'label' => 'swisscovery without CDI',
					'rtype' => 'exclude:journals;articles;newspaper_articles',		/* for media types */
				),
				'dn_and_ci' => array(
					'scope' => 'DN_and_CI',
					'tab' => '41SLSP_LIB_DN_CI',
					'label' => 'swisscovery',
					'rtype' => 'exclude:books;book_chapters|include:journals',			/* for media types */
					'facet' => 'include:online_resources',
				),
			);
		}
		$cat = @trim(strip_tags($_GET['catalog']));
		$this->apiScope = ( empty($cat) ? $apiSearchScope : $cat );

		parent::__construct($this->apiLabel);
	}

	public function getQueryField($searchTerm = '', $qField = 'any')
	{
		if ( $searchTerm = @strtoupper(trim(urldecode($searchTerm))) ) {
			$ary = array();
			preg_match('/\d{4}\s*\-*\s*\d{4}|\d{4}\s*\-*\s*\d{3}X/',$searchTerm,$ary);
			return ( @intval($ary[0]) ? 'issn' : $qField );
		}
		return $qField;
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$apiUrlAry = array(
			'q'      => $this->getQueryField($searchTerm) . ',contains,' . urldecode( empty($searchTerm) ? $this->searchTermOrig : $searchTerm ),
			'offset' => $searchOffset,
			'limit'  => $this->searchLimit,
			'lang'   => substr($this->apiLanguage,0,2),
			'sort'   => 'rank',
			'getMore'        => 0,
			'pcAvailability' => 'true',
			'conVoc'         => 'true',
			'skipDelivery'   => 'true',
			'disableSplitFacets' => 'true',
			'vid'    => $this->apiVid,
			'inst'   => strtok($this->apiVid.':',':'),
			'tab'    => $catAry['tab'],
			'scope'  => $catAry['scope'],
			'apikey' => $this->apiKey,
		);
		$apiUrl = 'https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?' . http_build_query($apiUrlAry,'','&',PHP_QUERY_RFC3986);
		if ( @!empty($catAry['rtype']) ) {
			foreach( explode('|',$catAry['rtype']) as $tmp ) {
				$cmd = 'qInclude';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'qExclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				$apiUrl .= '&' . $cmd . '=facet_rtype%2Cexact%2C' . implode('%7C%2C%7Cfacet_rtype%2Cexact%2C',$ary);
			}
			// note that 'multiFacets=facet_rtype,...' would work too
		}
		if ( @!empty($catAry['facet']) ) {
			foreach( explode('|',$catAry['facet']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				$apiUrl .= '&multiFacets=facet_tlevel%2C' . $cmd . '%2C' . implode('%7C%2C%7Cfacet_tlevel%2C' . $cmd . '%2C',$ary);
			}
		}
		return $apiUrl;
		// for example:
		// https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?vid=41SLSP_LIB%3AGO&tab=CDI&scope=CentralIndex&q=any%2Ccontains%2Crainbow&lang=eng&offset=0&limit=10&sort=rank&pcAvailability=true&getMore=0&conVoc=true&inst=41SLSP_LIB&skipDelivery=true&disableSplitFacets=true&apikey=7777....ffff
		// or general/union
		// https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=any%2Ccontains%2CWater%20Research&offset=0&limit=10&lang=en&sort=rank&getMore=0&pcAvailability=true&conVoc=true&skipDelivery=true&disableSplitFacets=true&vid=41SLSP_NETWORK:VU1_UNION&inst=41SLSP_NETWORK&tab=DiscoveryNetwork&scope=DiscoveryNetwork&apikey=7777....ffff
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		// Corresponding to the intended API request, also the human user must have/get a corresponding link to the online search page:
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$webUrlAry = array(
			'query'  => $this->getQueryField($searchTerm) . ',contains,' . urldecode($searchTerm),
			'offset' => $searchOffset,
			'limit'  => $this->searchLimit,
			'lang'   => substr($this->apiLanguage,0,2),
			'vid'    => $this->apiVid,
			'tab'    => $catAry['tab'],
			'search_scope'  => $catAry['scope'],
		);
		$webUrl = 'https://' . ( stripos('/union/',$this->apiScope) || stripos('/union/',$catAry['scope']) ? '' : 'lib4ri.' );
		$webUrl .= 'swisscovery.slsp.ch/discovery/search?' . http_build_query($webUrlAry,'','&',PHP_QUERY_RFC3986);
		if ( @!empty($catAry['rtype']) ) {
			foreach( explode('|',$catAry['rtype']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				foreach( $ary as $i => $tmp /* name of resource type */ ) {
					$webUrl .= '&mfacet=rtype,' . $cmd . ',' . $tmp . ',' . strval($i+1);
				}
			}
		}
		if ( @!empty($catAry['facet']) ) {
			foreach( explode('|',$catAry['facet']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				foreach( $ary as $i => $tmp /* name of resource type */ ) {
					$webUrl .= '&mfacet=tlevel,' . $cmd . ',' . $tmp . ',' . strval($i+1);
				}
			}
		}
		return $webUrl;
		// for example:
		// https://lib4ri.swisscovery.slsp.ch/discovery/search?query=any,contains,rainbow&tab=CDI&search_scope=CentralIndex&vid=41SLSP_LIB:GO&lang=en&offset=0
		// or general/union
		// https://swisscovery.slsp.ch/discovery/search?query=any,contains,Water%20Research&tab=DiscoveryNetwork&search_scope=DiscoveryNetwork&vid=41SLSP_NETWORK:VU1_UNION&offset=0
	}
}

class apiQueryJournal extends apiQueryUtils
{
	// see https://developers.exlibrisgroup.com/blog/understanding-and-using-the-primo-search-api-as-a-guest-in-the-developer-network/

	public $apiVid;
	public $apiCatAry;

	public function __construct($apiSearchTab = '')		// $apiSearchTab: jsearch_slot OR 41SLSP_LIB_DN_CI
	{
		$this->apiName = 'journal';		// lower-case!
		$this->apiHost = 'slsp';
		$this->apiLabel = 'Journal List';		//  will serve a ID for this type of API request!
		$this->apiKey = @strval( W_S_API_SLSP_KEY );
		$this->apiVid = '41SLSP_LIB:GO';

		$this->apiCatAry = array(	 /* Catalogues, with corresponding search keys/parameters: */
			'dn_and_ci' => array(
				'scope' => 'DN_and_CI',
				'tab' => '41SLSP_LIB_DN_CI' /* ( $apiSearchTab != '41SLSP_LIB_DN_CI' ? 'jsearch_slot' : '41SLSP_LIB_DN_CI' ) */ ,
				'label' => 'swisscovery',
				'rtype' => 'include:journals',
				'facet' => 'include:online_resources',
			),
			'myinstitution' => array(
				'scope' => 'MyInstitution',
				'tab' => 'LibraryCatalog',
				'label' => 'swisscovery',
				'rtype' => 'include:journals|include,newspapers',
				'facet' => 'include:online_resources',
			),
		);
		$this->apiScope = 'MyInstitution'; // = 'DN_and_CI';		// determines which one to use from apiCatAry

		parent::__construct($this->apiLabel);
	}
	
	public function getQueryField($searchTerm = '', $qField = 'any')
	{
		if ( $searchTerm = @strtoupper(trim(urldecode($searchTerm))) ) {
			$ary = array();
			preg_match('/\d{4}\s*\-*\s*\d{4}|\d{4}\s*\-*\s*\d{3}X/',$searchTerm,$ary);
			return ( @intval($ary[0]) ? 'issn' : $qField );
		}
		return $qField;
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$apiUrlAry = array(
			'q'      => $this->getQueryField($searchTerm,'title') . ',contains,' . $searchTerm,
			'offset' => $searchOffset,
			'limit'  => min( $this->searchLimit, 20 ),
			'lang'   => substr($this->apiLanguage,0,2),
			'sort'   => 'rank',
			'getMore'        => 0,
			'pcAvailability' => 'true',
			'conVoc'         => 'true',
			'skipDelivery'   => 'true',
			'disableSplitFacets' => 'true',
			'vid'    => $this->apiVid,
			'inst'   => strtok($this->apiVid.':',':'),
			'tab'    => $catAry['tab'],
			'scope'  => $catAry['scope'],
			'apikey' => $this->apiKey,
		);
		$apiUrl = 'https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?' . http_build_query($apiUrlAry,'','&',PHP_QUERY_RFC3986);
		if ( @!empty($catAry['rtype']) ) {
			foreach( explode('|',$catAry['rtype']) as $tmp ) {
				$cmd = 'qInclude';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'qExclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				$apiUrl .= '&' . $cmd . '=facet_rtype%2Cexact%2C' . implode('%7C%2C%7Cfacet_rtype%2Cexact%2C',$ary);
			}
			// note that 'multiFacets=facet_rtype,...' would work too
		}
		if ( @!empty($catAry['facet']) ) {
			foreach( explode('|',$catAry['facet']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				$apiUrl .= '&multiFacets=facet_tlevel%2C' . $cmd . '%2C' . implode('%7C%2C%7Cfacet_tlevel%2C' . $cmd . '%2C',$ary);
			}
		}
		return $apiUrl;
		// for example:
		// https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=title%2Ccontains%2CHydrogen&offset=0&limit=10&lang=en&sort=rank&getMore=0&pcAvailability=true&conVoc=true&skipDelivery=true&disableSplitFacets=true&vid=41SLSP_LIB%3AGO&inst=41SLSP_LIB&tab=jsearch_slot&scope=DN_and_CI&apikey=7777....ffff&qInclude=facet_rtype%2Cexact%2Cjournals
		// https://lib4ri.swisscovery.slsp.ch/primaws/rest/pub/pnxs?blendFacetsSeparately=false&disableCache=false&getMore=0&inst=41SLSP_LIB&journals=any,Hydrogen&lang=en&limit=10&newspapersActive=false&newspapersSearch=false&offset=0&pcAvailability=true&q=any,contains,Hydrogen&qExclude=&qInclude=&rapido=false&refEntryActive=true&rtaLinks=true&searchInFulltextUserSelection=true&skipDelivery=Y&sort=rank&tab=jsearch_slot&scope=DN_and_CI&vid=41SLSP_LIB:GO
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		// Corresponding to the intended API request, also the human user must have/get a corresponding link to the online search page:
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$webUrlAry = array(
			'query'  => $this->getQueryField($searchTerm,'title') . ',contains,' . $searchTerm,
			'offset' => $searchOffset,
			'limit'  => $this->searchLimit,
			'lang'   => substr($this->apiLanguage,0,2),
			'vid'    => $this->apiVid,
			'tab'    => $catAry['tab'],
			'search_scope'  => $catAry['scope'],
			'journals'      => 'title,' . $searchTerm,
		);
		$webUrl = 'https://lib4ri.swisscovery.slsp.ch/discovery/search?' . http_build_query($webUrlAry,'','&',PHP_QUERY_RFC3986);
		if ( @!empty($catAry['rtype']) ) {
			foreach( explode('|',$catAry['rtype']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				foreach( $ary as $i => $tmp /* name of resource type */ ) {
					$webUrl .= '&mfacet=rtype,' . $cmd . ',' . $tmp . ',' . strval($i+1);
				}
			}
		}
		if ( @!empty($catAry['facet']) ) {
			foreach( explode('|',$catAry['facet']) as $tmp ) {
				$cmd = 'include';
				if ( substr($tmp,0,8) == 'exclude:' ) {
					$tmp = substr($tmp,8);
					$cmd = 'exclude';
				} elseif ( substr($tmp,0,8) == 'include:' ) {
					$tmp = substr($tmp,8);
				}
				$ary = array_filter(array_map('trim',explode(';',$tmp)));
				foreach( $ary as $i => $tmp /* name of resource type */ ) {
					$webUrl .= '&mfacet=tlevel,' . $cmd . ',' . $tmp . ',' . strval($i+1);
				}
			}
		}
		return $webUrl;
		// for example:
		// https://lib4ri.swisscovery.slsp.ch/discovery/jsearch?query=title,contains,Hydrogen&tab=jsearch_slot&vid=41SLSP_LIB:GO&offset=0&journals=title,Hydrogen
	}
}







class apiQueryDORA extends apiQueryLib4ri
{
	// see https://www.wiki.lib4ri.ch/display/HEL/Technical+details+of+DORA

	public $apiSolrSimple;

	public function __construct($apiInstitute)
	{
		if ( !empty($apiInstitute) ) {
			$this->apiScope = $apiInstitute;
		}
		$this->apiLabel = 'DORA' . ( empty($this->apiScope) ? '' : '/'.( strlen($this->apiScope) < 4 ? strtoupper($this->apiScope) : ucFirst(strtolower($this->apiScope)) ) );
		$this->apiKey = '';
		$this->apiSolrSimple = 'mods_titleInfo_title_mt^5 mods_abstract_ms^2 dc.creator^2 mods_extension_originalAuthorList_mt^2 dc.contributor^1 dc.type^1 catch_all_MODS_mt^1';
		parent::__construct($this->apiLabel);
	}

	public function buildQuery($searchTermEnc = '')
	{
		$searchQuery = 'dc.title:' . ( empty($searchTermEnc) ? '*' : '('.$searchTermEnc.')' ); // aux
		if ( @!empty($this->apiSolrSimple) ) {
			$searchQuery = str_replace(' ','%20OR%20',preg_replace('/\s/',' ',$this->apiSolrSimple));
			$searchQuery = implode(':'.( empty($searchTermEnc) ? '*' : '('.$searchTermEnc.')' ).'^',explode('^',$searchQuery));
		}
		return $searchQuery;
	}
	
	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		if ( strpos($searchTermEnc,'%20') ) {
			$searchTermEnc = '(' . $searchTermEnc . ')';		// in DORA we need brackets for multiple search terms!
		}
		// to get the citiation of a given amount of pids:
		$apiUrl = 'https://www.dora.lib4ri.ch/' . ( empty($this->apiScope) ? '' : strtolower($this->apiScope).'/' );
	//	$apiUrl .= 'islandora/search/json_meta/PID:' . ( empty($this->apiScope) ? '*' : strtolower($this->apiScope).'%5c%3a*' );
	//	$apiUrl .= '?limit=' . $this->searchLimit . '&f[3]=' . $this->buildQuery($searchTermEnc);
		$apiUrl .= 'islandora/search/json_meta/' . $this->buildQuery($searchTermEnc) . '?limit=' . $this->searchLimit . '&offset=' . $searchOffset;
		return $apiUrl;
		// for example:
		// https://www.dora-dev.lib4ri.ch/eawag/islandora/search/json_meta/PID:eawag%5c%3a*?limit=20&f[2]=mods_titleInfo_title_mt:(hydro)^5%20OR%20mods_abstract_ms:(hydro)^2%20OR%20dc.creator:(hydro)^2%20OR%20mods_extension_originalAuthorList_mt:(hydro)^2%20OR%20dc.contributor:(hydro)^1%20OR%20dc.type:(hydro)^1%20OR%20catch_all_MODS_mt:(hydro)^1%20OR%20PID_et:(hydro)^7
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		if ( strpos($searchTermEnc,'%20') ) {
			$searchTermEnc = '(' . $searchTermEnc . ')';		// in DORA we need brackets for multiple search terms!
		}
		$webUrl = 'https://www.dora.lib4ri.ch/' . ( empty($this->apiScope) ? '' : strtolower($this->apiScope).'/' );
		$webUrl .= 'islandora/search/' . $this->buildQuery($searchTermEnc) . '?search_string=' . $searchTermEnc . '&extension=false';
		$webUrl .= '&limit=' . $this->searchLimit . '&offset=' . $searchOffset;		// supported already!?
		return $webUrl;
		//	for example:
		//	https://www.dora.lib4ri.ch/eawag/islandora/search/mods_titleInfo_title_mt:%28hydro%29%5E5%20OR%20mods_abstract_ms:%28hydro%29%5E2%20OR%20dc.creator:%28hydro%29%5E2%20OR%20mods_extension_originalAuthorList_mt:%28hydro%29%5E2%20OR%20dc.contributor:%28hydro%29%5E1%20OR%20dc.type:%28hydro%29%5E1%20OR%20catch_all_MODS_mt:%28hydro%29%5E1%20OR%20PID_et:%28hydro%29%5E7?search_string=hydro&extension=false
	}
}


/*
$getJsonScopus = new apiQueryScopus();
echo print_r( $getJsonScopus->queryNow( 'hydro' ), 1 ) . "\r\n";
echo print_r( $getJsonScopus->apiUrl( 'hydro' ), 1 );

$getJsonSLSP = new apiQuerySLSP('');
echo print_r( $getJsonSLSP->queryNow( 'hydro' ), 1 ) . "\r\n";
echo print_r( $getJsonSLSP->webUrl( 'hydro' ), 1 );

$getJsonDORA = new apiQueryDORA('eawag');
echo print_r( $getJsonDORA->queryNow( 'hydro' ), 1 ) . "\r\n";
echo print_r( $getJsonDORA->apiUrl( 'hydro' ), 1 );
*/


class apiQueryCurrency extends apiQueryUtils
{	/*
		We need to know the exchange rate to determine whether or not to show Lib4RI funding hint depending on the APC price.

		Currently rates are take from an UBS page, intended cage time 1h.
			https://secure.ubs.com/global/en/quotes.html
		There are ( free) APIs for this, check/overview:
			https://geekflare.com/finance/best-currency-exchange-apis/
			https://secure.ubs.com/app/ABU/4/QCoreWeb/GRT_4_Dispatch/4.0.0/pub/pg_mi/main/js/int/x/miCurrencyOverview.do	<== see source-code
		Example:
			https://www.exchangerate-api.com/	( need to register )
			https://exchangeratesapi.io/	( 250 request free per month, so cache time of >= 3h possible )
	*/

	public $request = '';
	public $exRateAry = array();
	protected $cacheTime = 5400;	// in seconds, cached currently as JSON file
	protected $jsonType = '';		// to influence how the output should be

	protected function tuneRequest($request = '')	// to turn '€' or 'E/CHF' into EUR/CHF
	{
		if ( empty($request) ) { return ''; }

		$ary = array_map('trim',explode('/',$request));
		$ary = array_map('strtoupper',array_map('urlencode',$ary));
		foreach( $ary as $idx => $cur ) {
			if ( $cur == 'DOLLAR' || $cur == 'S' || $cur == '%24' ) {
				$ary[$idx] = 'USD';
			} elseif ( $cur == 'EURO' || $cur == 'E' || $cur == '%E2%82%AC' ) {
				$ary[$idx] = 'EUR';
			} elseif ( $cur == 'POUND' || $cur == 'P' || $cur == '%C2%A3' ) {
				$ary[$idx] = 'GBP';
			} elseif ( $cur == 'YEN' || $cur == 'Y' || $cur == '%C2%A5' ) {
				$ary[$idx] = 'JPY';
			}
		}
		if ( empty($ary[1]) ) {
			$ary[1] = ( $ary[0] == 'CHF' ? 'USD' : 'CHF' );
		}
		$this->request = urldecode(implode('/',$ary));
		return $this->request;
	}
	
	public function __construct( $jsonType = '' )	// empty for file DL
	{
		$this->apiName = 'currency';
		$this->apiLabel = ( empty($apiLabel) ? 'Currency List' : $apiLabel );
		$this->jsonType = $jsonType;
		parent::__construct($this->apiLabel);
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		return 'https://secure.ubs.com/app/ABU/4/QCoreWeb/GRT_4_Dispatch/4.0.0/pub/pg_mi/main/js/int/x/miCurrencyOverview.do';	// = portion of an HTML page
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		return 'https://secure.ubs.com/global/en/quotes.html';
	}

	public function getRateData()		// will assign $this->exRateAry
	{
		$curFile = '/tmp/currency-exchange-rates.chache.json';
		$exRateAry = array( /* exchange => array( Price | Change | Day chg. % | Low | High | 52W low | 52W high ) */ );

		if ( $this->cacheTime > 0 && @filesize($curFile) && ( time() - filemtime($curFile) ) < $this->cacheTime ) {
			$ary = @json_decode( file_get_contents($curFile), true );
			if ( is_array($ary) && sizeof($ary) > 3 ) {
				$this->exRateAry = $ary;
				return;
			}
		}
		$browser = strval( ( time() >> 23  ) - 97 );	// assuming all 3 months a new version
		$browser = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:'.$browser.'.0) Gecko/20100101 Firefox/'.$browser.'.0';
		$htmlCode = $this->makeQuery( $this->apiUrl(), 10, $browser );

		if ( $pos = strpos($htmlCode,'aceq_table') ) {
			$htmlCode = substr(strchr(substr($htmlCode,$pos,strpos($htmlCode,'</table>')-$pos),'>'),1);
			foreach( explode('</tr>',$htmlCode) as $trArea ) {
				$trAry = array_map('strip_tags',explode('</td>',$trArea));
				if ( strpos($trAry[0],'/') ) {
					$exRateAry[array_shift($trAry)] = array_slice($trAry,0,7);
				/*	$exRateAry to hold e.g. :		[EUR/CHF] => Array(
														[0] => 1.0156
														[1] => -0.02274
														[2] => &minus;2.19%
														[3] => 1.01308
														[4] => 1.04187
														[5] => 0.99728
														[6] => 1.09913
													)							*/
				}
			}
			if ( sizeof($exRateAry) > 3 ) {
				file_put_contents($curFile,json_encode($exRateAry,JSON_PRETTY_PRINT));
			}
		}
		if ( sizeof($exRateAry) > 3 ) {	$this->exRateAry = $exRateAry; }
	}
	
	public function queryNow($request = /* 'USD/CHF' */ '' )	// note with 'USD' only you will just get the rate as number
	{
		if ( empty($request) ) {
			$this->setupParam();
            if ( !empty($this->searchTerm) ) {
				if ( strtoupper($this->searchTerm) == 'CHF' ) {
					return 1.0;
				}
				$request = $this->searchTerm;
			}
		}
		$this->getRateData();
		
		$this->tuneRequest( $request . ( @strpos($request,'/') ? '' : ( empty($request) ? '' : '/CHF' ) ) );	// will update $this->request
	
		if ( empty(trim($request)) || !isset($this->exRateAry[$this->request]) ) {
			return $this->jsonOutput( $this->exRateAry, $this->jsonType );
		}
		if ( $this->jsonType == 'float' || $this->jsonType == 'double' ) { // special case
			return array_shift($this->exRateAry[$this->request]);		// intentionally no JSON, same as $this->jsonOutput( ARRAY/DATA, 'none' )
		} 
		return $this->jsonOutput( array( $this->request => $this->exRateAry[$this->request] ), $this->jsonType );
	}
}


class apiQueryWikipedia extends apiQueryUtils
{
	// NOT FINAL YET - WORK IN PROGRESS!
	// Main issue:
	// The currently used/referenced API module from Wikimedia does not return the total amount of search results
	
	// see https://en.wikipedia.org/w/api.php
	// see https://www.mediawiki.org/wiki/API:Page_info_in_search_results

	public function __construct($apiLabel = '')
	{
		$this->apiName = 'wikipedia';
		$this->apiHost = 'wikimedia';
		$this->apiLabel = 'Wikipedia';		//  will serve a ID for this type of API request!
		$this->searchLimit = 10;
		$this->apiKey = '';
		$this->apiScope = '';
		parent::__construct($this->apiLabel);
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$apiUrlAry = array(
			'action' => 'query',
			'format' => 'json',
			'generator' => 'prefixsearch',
			'prop' => 'info|pageprops|pageimages|description|extracts',
			'gpssearch' => $searchTerm,
			'gpslimit' => max( min( $this->searchLimit, 30 ), 1 ) + 1,	/* +1 because of the 'may refer to' matter */
			'ppprop' => 'displaytitle',
			'inprop' => 'url',
			'redirects' => '',
			'exintro' => '',
			'explaintext' => '',
		/*	'piprop' => 'thumbnail',
			'pilimit' => max( min( $this->searchLimit, 30 ), 1 ) + 1,
			'pithumbsize' => '80',			*/
		);
		return ( 'https://en.wikipedia.org/w/api.php?' . http_build_query($apiUrlAry,'','&',PHP_QUERY_RFC3986) );
		// for example:
		// https://en.wikipedia.org/w/api.php?action=query&format=json&gpssearch=Deuterium&gpslimit=10&generator=prefixsearch&ppprop=displaytitle&redirects=&prop=info|pageprops|pageimages|description|extracts&exintro&explaintext&inprop=url
		// https://en.wikipedia.org/w/api.php?action=query&format=json&srsearch=Deuterium&list=search&srinfo=totalhits&srlimit=10&redirects=&prop=info|description&inprop=url
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		return( 'https://en.wikipedia.org/wiki/' . rawurlencode($searchTerm) );
		// for example:
		// https://en.wikipedia.org/wiki/Hydrogen
	}

	public function makeHtml($jsonData = null, $maxLength = 150, $cssItem = 'csl-entry')
	{
		$htmlAry = [];
		$jsonAry = ( is_array($jsonData) ? $jsonData : json_decode( ( is_string($jsonData) ? $jsonData : json_encode($jsonData) ), true ) );
		$max = $this->searchLimit;		// to count, because of the 'may refer to' matter
		foreach( $jsonAry['query']['pages'] as $wId => $wAry ) {
			$tmp = trim( strip_tags( urldecode( $wAry['extract'] ) ) ); // rather for safety
			if ( substr($tmp,-14) == ' may refer to:' ) {
				continue;		// special case, this is listed as normal result!(?) Link in the footer will do the same
			}
			if ( strlen($tmp) > ($maxLength + 20) ) {
				$tmp = rtrim(substr($tmp,0,$maxLength+10));
				if ( $pos = strrpos($tmp,' ') ) { $tmp = substr($tmp,0,$pos); }
				$tmp = rtrim($tmp,":;,. \\\r\t\n\v\x00") . '...';
			}
			$desc = trim( strip_tags( urldecode( $wAry['description'] ) ) ); // rather for safety
			$tmp = '<a href="' . $wAry['fullurl'] . '" target="_blank">' . $wAry['title'] . ( empty($desc) ? '' : ' ('.lcfirst($desc).')' ) . '</a>: ' . $tmp;
			$htmlAry[] = '<div class="' . $cssItem . '" style="display:inline-block;">' . $tmp . '</div>';
			 
			if ( ( --$max ) < 1 ) { break; }
		}
		return implode('',$htmlAry);
	}

	public function getNumFound($apiUrl = '')			/* no way found to get this with the inital request - to be tuned */
	{
		if ( empty($apiUrl) ) {
			$searchTerm = $this->getSearchTerm($searchTerm);
			$apiUrl = 'https://en.wikipedia.org/w/api.php?action=query&format=json&srsearch='. $searchTerm .'&list=search&srinfo=totalhits&srlimit=1&redirects=';
		}
		$jsonAry = json_decode( $this->makeQuery($apiUrl), true );
		return @intval($jsonAry['query']['searchinfo']['totalhits']);
	}
		
}


/*
// for interal usage (but has some API/web features too):
$reqTerm = '$';
$apiTool = new apiQueryCurrency( 'float' );
$exData = $apiTool->queryNow( $reqTerm );
$reqTerm = $apiTool->request;	// update/safer!
if ( !empty($reqTerm) && is_array($exData) ) {
	$exData = ( @isset($exData[$reqTerm]) ? $exData[$reqTerm][0] : 'n/a' );
}
echo "Exchange Rate " . $reqTerm . ": " . print_r( $exData, 1 );
*/
