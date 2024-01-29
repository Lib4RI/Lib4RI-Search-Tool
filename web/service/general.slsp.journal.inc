<?php

class apiQueryJournal extends apiQuerySLSP
{
	// see https://developers.exlibrisgroup.com/blog/understanding-and-using-the-primo-search-api-as-a-guest-in-the-developer-network/

	public $apiVid;
	public $apiCatAry;
	public $apiOptAry;

	public function __construct($apiSearchTab = '', $apiOptionList = '')		// $apiSearchTab: jsearch_slot OR 41SLSP_LIB_DN_CI
	{
		$this->apiName = 'journal';		// lower-case!
		$this->apiHost = 'slsp';
		$this->apiLabel = 'Journal List';		//  will serve a ID for this type of API request!
		$this->apiKey = @strval( W_S_API_SLSP_KEY );
		$this->apiVid = ( $apiSearchTab == 'slsp-wide' ? '41SLSP_NETWORK:VU1_UNION' : '41SLSP_LIB:GO' );
		
		$apiOptionList = strtolower( empty($apiOptionList) ? @strip_tags($_GET['option']) : $apiOptionList );
		$this->apiOptAry = array_map('trim',explode(',',$apiOptionList));

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
				'rtype' => 'include:journals|include:newspapers',
				'facet' => 'include:online_resources',
			),
			'slsp-wide' => array(
				'scope' => 'DN_and_CI',
				'tab' => '41SLSP_NETWORK',
				'label' => 'swisscovery',
				'rtype' => 'include:journals|include:newspapers',
				'facet' => 'include:online_resources',
			),
		);
		$cat = @trim(strip_tags($_GET['catalog']));			// optional, usually empty
		$this->apiScope = ( empty($cat) ? $apiSearchTab : $cat );		// determines which one to use from apiCatAry

		if ( $this->apiScope == 'slsp-wide' ) {
			$this->apiVid = '41SLSP_NETWORK:VU1_UNION';
		}

		parent::__construct($this->apiLabel);
	}

	/*
		Trivial word trimmer, to optimze search resulsts
		inspiration: lib4ridora/includes/advanced_search.funding.inc / lib4ridora_word_solr()
	*/
	protected function wordTrim( $word = '' )
	{
		if ( strlen($word) < 5 ) {
			return $word;	// considered as too short
		}
		if ( strpos(' 1234567890',substr($word,0,1)) || strpos(' 1234567890',substr($word,-1)) ) {
			return $word;	// ending or starting with a number
		}
		if ( strtolower(substr($word,-1)) == substr($word,-1) ) {
			return $word;		// last case is uppercase, could be an acronym, query operator
		}
		$wordLC = ( $ignore_case ? strtolower($word) : $word );
		// plural handling:
		if ( strlen($word) > 5 && substr($wordLC,-3) == 'ves' ) {		// e.g: shelf/shelves
			$word = substr($word,0,-3) . 'f';
			$wordLC = substr($wordLC,0,-3) . 'f';
		} elseif ( strlen($word) > 3 && substr($wordLC,-1) == 's' ) {		// no plural es/s
			$len = ( strlen($word) > 4 && substr($wordLC,-2) == 'es' ) ? 2 : 1;
			$word = substr($word,0,-$len);
			$wordLC = substr($wordLC,0,-$len);
		}
		return $word;		// early returm (due to lack of tuning)
	/*
		$endAry = array( // for ending to replace, NOT TUNED (YET)
			'ing'  => '',
			'ied'  => 'y',
			'ed'   => '',
			'tal'  => 't',
			'tive' => 't',
			'ive'  => '',
			'ial'  => '',
			'ish'  => '',
			'ious' => '',
			'ous'  => '',
			'ical' => '',
			'ful'  => '',
			'sion' => 's',
			'tion' => 't',
			'ion'  => '',
			'ity'  => '',
			'ant'  => '',
			'ment' => '',
			'ness' => '',
			'ship' => '',
			'ism'  => '',
			'sis'  => '',
			'age'  => '',
			'ery'  => '',
			'our'  => 'o',
			'or'   => 'o',
			'or'   => 'o',
		);
		foreach( $endAry as $eRep => $eNew ) {
			$len = strlen($eRep);
			if ( strlen($word) < ($len + 2) ) { continue; }
			for($i=$len;$i>1;$i--) {
				if ( substr($wordLC,-$i) == substr($eRep,0,$i) ) {
					return ( rtrim( substr($word,0,-$i) . $eNew, 'aeijouy') );
				}
			}
		}
		*/
		return rtrim($word,'aeijouy');
	}

	protected function tuneSearchTerm( $wordList = null, $wordSuffix = '' )		// TEST
	{
		// inspiration: /var/www/html/sites/all/modules/lib4ridora/includes/advanced_search.funding.inc / lib4ridora_word_solr()
		$wordAry = array_filter( is_array($wordList) ? $wordList : explode(' ',$wordList) );
		foreach( $wordAry as $idx => $word ) {
			$wordAry[$idx] = $this->wordTrim($word) . $wordSuffix;
		}
		// Gerneral/all-API tuning would be here below:
		return implode(' ',$wordAry);
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$apiUrlAry = array(
			'q'      => '',	/* TBA below */
			'offset' => $searchOffset,
			'limit'  => ( ( intval($this->searchLimit) > 499 ) ? 99999 : min( $this->searchLimit, 20 ) ),
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

		// Which kind of title search? Untrestricted would include ceased journals and earlier/later titles.
		$apiUrlAry['q'] = $this->getQueryField( $searchTerm, ( in_array('earlier',$this->apiOptAry) ? 'title' : 'lds99' ) );
		$tmp = ( ( $this->apiHost == 'slsp' && in_array($apiUrlAry['q'],['title','lds99']) ) ? $this->tuneSearchTerm($searchTerm,'*') : $searchTerm );
		$apiUrlAry['q'] .= ',contains,' . $tmp;
		if ( !in_array('ceased',$this->apiOptAry) ) {
			$apiUrlAry['q'] .= ',NOT;lds100,contains,Ceased';
		}

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
			'query'  => $this->apiQueryMain,
			'offset' => $searchOffset,
			'limit'  => $this->searchLimit,
			'lang'   => substr($this->apiLanguage,0,2),
			'vid'    => $this->apiVid,
			'tab'    => $catAry['tab'],
			'search_scope'  => $catAry['scope'],
			'journals'      => 'title,' . $searchTerm,
		);
		
		// Which kind of title search? Untrestricted would include ceased journals and earlier/later titles.
		$webUrlAry['query'] = $this->getQueryField( $searchTerm, ( in_array('earlier',$this->apiOptAry) ? 'title' : 'lds99' ) );
		$tmp = ( ( $this->apiHost == 'slsp' && in_array($webUrlAry['query'],['title','lds99']) ) ? $this->tuneSearchTerm($searchTerm,'*') : $searchTerm );
		$webUrlAry['query'] .= ',contains,' . $tmp;
		if ( !in_array('ceased',$this->apiOptAry) ) {
			$webUrlAry['query'] .= ',NOT;lds100,contains,Ceased';
		}

		$webUrl = 'https://' . ( ( $this->apiScope == 'slsp-wide' ) ? '' : 'lib4ri.' );
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
		// https://lib4ri.swisscovery.slsp.ch/discovery/jsearch?query=title,contains,Hydrogen&tab=jsearch_slot&vid=41SLSP_LIB:GO&offset=0&journals=title,Hydrogen
	}
}
