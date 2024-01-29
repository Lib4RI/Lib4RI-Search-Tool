<?php

class apiQueryWoS extends apiQueryWebTool
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
		$apiUrl = 'https://wos-api.clarivate.com/api/woslite/?usrQuery=' . ( $this->searchTermType != 'doi' ? 'ALL' : 'DO' ) . '%3d(' . $searchTermEnc . ')';
		return ( $apiUrl . '&databaseId=WOS&count=' . $this->searchLimit . '&firstRecord=' . $searchOffset );
		// for example (with cURL):
		// curl -X GET "https://wos-api.clarivate.com/api/woslite/?databaseId=WOS&usrQuery=TI%3d(hydro)%20OR%20AB%3d(hydro)%20OR%20AU%3d(hydro)&count=10&firstRecord=1" -H "accept: application/json" -H "X-ApiKey: a123....789f"
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		// $searchOffset = ...	<== offset handling unknown for web links
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$searchTermEnc = rawurlencode($searchTerm);
		return ( 'https://www.webofscience.com/wos/woscc/general-summary?query=' . ( $this->searchTermType != 'doi' ? 'ALL' : 'DO' ) . '%3d(' . $searchTermEnc . ')' );
		// for example:
		// https://www.webofscience.com/wos/woscc/general-summary?query=TI%3d(hydro)%20OR%20AB%3d(hydro)%20OR%20AU%3d(hydro)
	}
}
