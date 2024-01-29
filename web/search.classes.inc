<?php

@include_once('./search.config.inc');
@include_once('./search.traits.inc');


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

// ==================================================================================

class apiQueryUtils
{
	use apiQueryTraits;

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
	public $searchTermType;	// like 'doi', 'issn', 'isbn'
	public $searchLimit;	// numerical
	public $searchOffset;	// numerical
	public $citeStyle = '' /* 'national-institute-of-health-research' */ ;
	public $formName;
	public $formTarget;
	// public $timeNeeded;	// to store how we stay in touch with the remote server - DISABLED with current implementation this rather the time to connect only.
	public $infoCurl;		// make this any array before makeQuery() to get tech. infos from cUrl.

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
		$this->citeStyle = W_S_CITEPROC_STYLE;
	}

	public function cleanTerm($term = '', $preProcess = ['cleanUrlInput','stripTags'] )
	{
		foreach( @array_slice($preProcess,0) as $funcName ) {
			if ( method_exists($this,$funcName) ) {
				$term = $this->$funcName($term);
			}
			elseif ( function_exists($funcName) ) {
				$term = $funcName($term);
			}
		}
		return trim( preg_replace('/\s+/',' ',$term) );
	}

	public function cleanTermOld($searchTerm = '', $tuning = ['urldecode','strip_tags'] )
	{
		// clean-up for/against quotes+tags:
	//	return implode(' ',array_map('trim',str_getcsv(preg_replace('/\s+/',' ',strip_tags(urldecode($searchTerm))),' ')));
		foreach( @array_slice($tuning,0) as $funcName ) {
			if ( function_exists($funcName) ) {
				$searchTerm = $funcName($searchTerm);
			}
		}
		$searchTerm = trim( preg_replace('/\s+/',' ',$searchTerm) );
		return $searchTerm;
	}

	public function size( $data = null ) // helper function to assist with update from 7.2 to 8.x
	{
		return ( @!is_countable($data) ? 0 : count($data) );	// background: https://www.php.net/manual/en/function.count.php
	}

	public function getDoi($term = '', $atBeginn = false, $checkLevel = 0, $textDelimter = [' ','?','|'] )	// will return array with found DOIs, or FALSE if none
	{
		$try = trim( /* strip_tags(urldecode($term)) */ $term ); // remind DOIs like 10.1002/(sici)1521-4095(199902)11:2<175::aid-adma175>3.0.co;2-%23
		if ( empty($try) || ( $atBeginn && intval($try) != 10 ) ) {	// assuming all DOIs start with 10.
			return false;
		}
		$tmp = trim(urldecode($try)); // just to test for most obvious tags
		foreach( array('<'.'?','<'.'script') as $tag ) {
			if ( stripos($tmp,$tag) !== false ) {		// minimum tag check, since strip_tags() will be too strong - to be tuned
				return false;
			}
		}
		foreach( $textDelimter as $chr ) {
			if ( empty($chr) || $chr == ' ' ) { continue; }
			$try = str_replace($chr,' ',$try);	// our reg.exp. only will treat spaces as delimiter
		}

		$try = str_ireplace( ['%252f','%2f','/'], ['!?9?!','!?8?!','!?7?!'], $try );	// custom encoding of (encoded) slash
		$regExAry = array(	/* DOI-detection based on https://www.crossref.org/blog/dois-and-matching-regular-expressions/ */
			'/\b10\.\d{4,9}!?[7-9]?!.*\s+/i',	/* sufficient DOI detection perhaps, if a user enters just '10.10456/' */
			'/\b10\.\d{4,9}!?[7-9]?![^\s]+\s+/i',
			'/\b10\.\d{4,9}!?[7-9]?![-._;()/:A-Z0-9]+\s+/i',
		);	// added \s+ everywhere as custom delimiter (or \b probably would work too)
		$ary = [];
		preg_match_all( $regExAry[max(min($checkLevel,sizeof($regExAry)-1),0)], $try.' ', $ary );

		if ( sizeof($ary[0]) ) {
			$ary[0] = str_ireplace( ['!?9?!','!?8?!','!?7?!'], ['%252f','%2f','/'], $ary[0] );	// revert custom slash-encoding
			return array_map( function($t) { return rtrim($t); }, array_values($ary[0]) ); // we have at least 1 DOI !
		}
		return false;
	}

	public function getIssn($term = '', $atBeginn = false)	// will return array with found ISSNs, or FALSE if none
	{
		if ( $try = trim( strip_tags( urldecode($term) ) ) ) {
			if ( !$atBeginn || strpos('+1234567890',$try[0]) ) {
				$try = urldecode( str_ireplace( ['&minus;','&#8222;','&mdash;','&#8212;','&ndash;','&#8211;'], ['-','-','-','-','-','-'], rawurlencode($try) ) );
				if ( strpos($try,'--') ) {
					$try = preg_replace('/\-{2,}/','-',$try);
				}
				$ary = [];
				preg_match_all('/\b\d{4}\s*\-\s*\d{4}(\.|\b)|\b\d{4}\s*\-\s*\d{3}[Xx](\.|\b)/',$try,$ary);	// allowing e.g. '1234 - 555X' (removing spaces below then)
				if ( sizeof($ary[0]) ) {
					return array_map( function($t) { return preg_replace('/\s+/','',$t); }, array_values($ary[0]) );
				}
			}
		}
		return FALSE;
	}

	public function cleanIssn($issn = '',$tuneFunc = 'strtoupper')		//  to clean/normalize a single ISSN
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
		if ( empty($searchTerm) ) {
			$searchTerm = @trim($this->searchTerm);
			if ( empty($searchTerm) ) {
				$searchTerm = @trim($_GET['find']);		// caution, may contain unwanted tags!
			}
		}
		if ( !intval($searchTerm) ) {	// decode if it is not starting with a number (hence no DOI, no ISSN/ISBN)
			while( $tmp = urldecode(rawurldecode($searchTerm)) ) {
				if ( $tmp == $searchTerm ) { break; }
				$searchTerm = $tmp;
			}
		}
		$ary = $this->getDoi($searchTerm,true);	// try!
		$type = '';
		if ( $got = @trim($ary[0]) ) {
			$type = 'doi';
		} else { // fast DOI check, looking for "10.1234/" in "10.1234/any-thing.more":
			$got = trim(strip_tags(urldecode($searchTerm))); // possibly tagged DOIs are checked, so now we can do this!

			if ( strpos('+1234567890',$got[0]) ) {
				$tmp = strtr($got,'[{(;:._)}]','||||||||||');	// accepted word/number limiters
				$ary = [];
				preg_match('/^\d+/', preg_replace('/[^\w|^\d]/','',$tmp) /* to only have 0-9 or a-z, space-less */, $ary );
				if ( @strlen($ary[0]) ) {
					if ( strlen($ary[0]) > 9 ) {
						$type = 'isbn'; // ISBN10 or ISBN13 probably/hopefully
						$got = strtok($tmp.'|','|');
					} elseif ( strlen($ary[0]) > 7 /* ISSN like 0123-4567 */ || strtoupper(substr($ary[0],7,1)) == 'X' /* ISSN like 0123-789X */ ) {
						$ary = $this->getIssn($got,true);
						if ( $tmp = @trim($ary[0]) ) {
							$type = 'issn';
							$got = $tmp;
						}
					}
				}
			}
		}
		if ( !empty($got) ) {
			if ( $this->searchTermType != $type ) {
				$this->searchTermType = $type;
			}
			if ( $this->searchTermOrig != $got ) {
				$this->searchTermOrig = $got;
				$this->searchTerm = $got;
			}
		}
		return $got;
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
			'lib4ricms' => 500,
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

		// $this->timeNeeded = floatval( 0.0 - microtime(true) ); // negative, tmp!
		$jsonCode = '';
		if ( $timeout > 0 ) {		// with cURL:
			$headerAry = $this->makeApiQueryHeader('',$userAgent);
			$cUrl = curl_init();
			curl_setopt($cUrl, CURLOPT_URL, $apiUrl );
			curl_setopt($cUrl, CURLOPT_HTTPHEADER, $headerAry['http']['header'] );
			curl_setopt($cUrl, CURLOPT_FOLLOWLOCATION, true );
			curl_setopt($cUrl, CURLOPT_RETURNTRANSFER, 1 );
			curl_setopt($cUrl, CURLOPT_TIMEOUT, $timeout );
			curl_setopt($cUrl, CURLOPT_CONNECTTIMEOUT, ( ( $timeout + 1 ) >> 1 ) );
			if ( ( $jsonCode = curl_exec($cUrl) ) && @is_array($this->infoCurl) ) {
				$this->infoCurl = curl_getinfo($cUrl);
			}
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
		} else { // with PHP own functions:
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
		}
		// $this->timeNeeded = ( microtime(true) + $this->timeNeeded );	// final, in milliseconds
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

class apiQueryWebTool extends apiQueryUtils
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

// ==================================================================================

// Including all inc-files with the (extended) API classes for available online-services we are going to contact.
// Convention: To properly load the extended classes we rely on the file name: the more dot-separated parts it has,
// the more extended the classes inside are assmed to be (resp. parent classes reside in files with less name parts).
$dirInc = './service/';
$typeInc = 'general';
$classAry = [];
foreach( @scandir($dirInc) as $classItem ) {
	if ( substr($classItem,-4) != '.inc' ) { continue; }
	if ( substr($classItem,0,strlen($typeInc)+1) != $typeInc.'.' ) { continue; }
	if ( !filesize($dirInc.$classItem) ) { continue; }
	$classAry[count(explode('.',$classItem))][] = $classItem;
}
for($e=2;$e<8;$e++) {
	if ( @empty($classAry[$e]) ) { continue; }
	foreach( $classAry[$e] as $classItem ) { include_once($dirInc.$classItem); }
}

// -----------------------------------------------------------------------------------
