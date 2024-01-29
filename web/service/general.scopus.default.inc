<?php

class apiQueryScopus extends apiQueryWebTool
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
		$apiUrl = 'https://api.elsevier.com/content/search/scopus?query=' . ( $this->searchTermType != 'doi' ? 'TITLE-ABS-KEY-AUTH' : 'DOI' ) . '(' . $searchTermEnc . ')';
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
		$webUrl = 'https://www.scopus.com/results/results.uri?src=s&st1=' . $searchTermEnc . '&sot=b&s=' . ( $this->searchTermType != 'doi' ? 'TITLE-ABS-KEY-AUTH' : 'DOI' ) . '(' . $searchTermEnc . ')';
		return( $webUrl . '&cc=' . $this->searchLimit . '&offset=' . $searchOffset . '&sid=new' );
		// for example:
		// https://www.scopus.com/results/results.uri?src=s&searchterm1=Filtration&sot=b&s=TITLE-ABS-KEY-AUTH(Filtration)&sid=new&cc=50&offset=51
		// https://www.scopus.com/results/results.uri?src=s&searchterm1=Filtration&sot=b&s=ALL(Filtration)&sid=new&cc=50&offset=51
	}
}
