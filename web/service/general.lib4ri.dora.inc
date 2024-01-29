<?php

class apiQueryDORA extends apiQueryWebTool
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
		// Islandora specific replacement required (see: /islandora_solr_search/includes/utilities.inc):
		$searchTermEnc = str_ireplace( ['/','%2f','+','%2b'], ['~slash~','~slash~','~plus~','~plus~'], $searchTermEnc );

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
		$searchTerm = urldecode($searchTerm); // ensure DEcoded, tmp!
		$apiUrl = 'https://www.dora.lib4ri.ch/' . ( empty($this->apiScope) ? '' : strtolower($this->apiScope).'/' ) . 'islandora/search/json_meta/';
		if ( $this->searchTermType == 'doi' ) { // special case
			$searchTermEnc = rawurlencode(rtrim($searchTerm,' *')) . '*';	//_mLt field supports a wildcard, but exclude it from being encoded!
			$apiUrl .= '?f[]=mods_identifier_doi_mlt:' . str_replace('%','%5c%',$searchTermEnc); // backslash injection before encodings!
			// for example: 
			// https://www.dora.lib4ri.ch/empa/islandora/search/json_meta/?f[]=mods_identifier_doi_mlt:10.1002%5c%2F%5c%28sici%5c%291521-4095%5c%28199902%5c%2911%5c%3A2%5c%3C175%5c%3A%5c%3Aaid-adma175%5c%3E3.0.co%5c%3B2-%5c%2523
		} else {
			$searchTermEnc = rawurlencode( strpos($searchTerm,' ') ? '('.$searchTerm.')' /* DORA needs brackets for multiple search terms */ : $searchTerm );
		//	$apiUrl .= 'PID:' . ( empty($this->apiScope) ? '*' : strtolower($this->apiScope).'%5c%3a*' );
		//	$apiUrl .= '?limit=' . $this->searchLimit . '&f[3]=' . $this->buildQuery($searchTermEnc);
			$apiUrl .= $this->buildQuery($searchTermEnc) . '?limit=' . $this->searchLimit . '&offset=' . $searchOffset;
			// for example:
			// https://www.dora-dev.lib4ri.ch/eawag/islandora/search/json_meta/PID:eawag%5c%3a*?limit=20&f[2]=mods_titleInfo_title_mt:(hydro)^5%20OR%20mods_abstract_ms:(hydro)^2%20OR%20dc.creator:(hydro)^2%20OR%20mods_extension_originalAuthorList_mt:(hydro)^2%20OR%20dc.contributor:(hydro)^1%20OR%20dc.type:(hydro)^1%20OR%20catch_all_MODS_mt:(hydro)^1%20OR%20PID_et:(hydro)^7
		}
		return $apiUrl;
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		$searchOffset = ( $searchOffset < 0 ? $this->searchOffset : $searchOffset );
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTerm = urldecode($searchTerm); // ensure DEcoded, tmp!
		$webUrl = 'https://www.dora.lib4ri.ch/' . ( empty($this->apiScope) ? '' : strtolower($this->apiScope).'/' ) . 'islandora/search/';
		if ( $this->searchTermType == 'doi' ) { // special case
			$searchTermEnc = rawurlencode(rtrim($searchTerm,' *')) . '*';	//_mLt field supports a wildcard, but exclude it from being encoded!
			$webUrl .= '?f[]=mods_identifier_doi_mlt:' . str_replace('%','%5c%',$searchTermEnc); // backslash injection before encodings!
			// for example: 
			// https://www.dora.lib4ri.ch/empa/islandora/search/?f[]=mods_identifier_doi_mlt:10.1002%5c%2F%5c%28sici%5c%291521-4095%5c%28199902%5c%2911%5c%3A2%5c%3C175%5c%3A%5c%3Aaid-adma175%5c%3E3.0.co%5c%3B2-%5c%2523
		} else {
			$searchTermEnc = rawurlencode( strpos($searchTerm,' ') ? '('.$searchTerm.')' /* DORA needs brackets for multiple search terms */ : $searchTerm );
			$webUrl .= $this->buildQuery($searchTermEnc) . '?search_string=' . $searchTermEnc . '&extension=false';
			$webUrl .= '&limit=' . $this->searchLimit . '&offset=' . $searchOffset;		// supported already!?
			//	for example:
			//	https://www.dora.lib4ri.ch/eawag/islandora/search/mods_titleInfo_title_mt:%28hydro%29%5E5%20OR%20mods_abstract_ms:%28hydro%29%5E2%20OR%20dc.creator:%28hydro%29%5E2%20OR%20mods_extension_originalAuthorList_mt:%28hydro%29%5E2%20OR%20dc.contributor:%28hydro%29%5E1%20OR%20dc.type:%28hydro%29%5E1%20OR%20catch_all_MODS_mt:%28hydro%29%5E1%20OR%20PID_et:%28hydro%29%5E7?search_string=hydro&extension=false
		}
		return $webUrl;
	}
}
