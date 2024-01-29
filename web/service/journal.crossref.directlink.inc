<?php

class apiQueryIssnCrossref extends apiQueryUtilsIssn
{
	// https://api.crossref.org/swagger-ui/index.html

	public function __construct()
	{
		$this->apiLabel = 'Crossref';
		parent::__construct($this->apiLabel);
	}

	public function apiUrl($searchTerm = '', $startAt = 0)
	{
		$searchTerm /* means ISSN here */ = implode(' ',array_map('trim',str_getcsv(preg_replace('/\s+/',' ',$searchTerm),' ')));	// 'anti-quotes' clean-up!(?)
		return ( 'https://api.crossref.org/' . $searchTerm );
		// for example: https://api.crossref.org/journals/1806-3713
	}

	public function webUrl($searchTerm = '', $startAt = 0)
	{
		$searchTerm /* means ISSN here */ = implode(' ',array_map('trim',str_getcsv(preg_replace('/\s+/',' ',$searchTerm),' ')));	// 'anti-quotes' clean-up!(?)
		return 'https://api.crossref.org/works?facet=license:*&filter=issn:' . $searchTerm;
		// for example:
		// https://api.crossref.org/works?facet=license:*&filter=issn:1806-3713		<=== json, not real web page!
	}
}
