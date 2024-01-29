<?php

class apiQuerySwisscovery extends apiQuerySLSP
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

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
	//	$searchTermEnc = rawurlencode($searchTerm);		// note: http_build_query will encode it
		$catAry = $this->apiCatAry[strtolower($this->apiScope)];
		$apiUrlAry = array(
			'q'      => $this->getQueryField($searchTerm,'any') . ',contains,' . urldecode( empty($searchTerm) ? $this->searchTermOrig : $searchTerm ),
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
			'query'  => $this->getQueryField($searchTerm,'any') . ',contains,' . urldecode($searchTerm),
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
