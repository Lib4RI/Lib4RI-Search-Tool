<?php

class apiQueryIssnDOAJold extends apiQueryUtilsIssn
{
	// see https://www.doaj.org/api/docs#!/CRUD_Journals/get_api_journals_journal_id

	public function __construct()
	{
		$this->apiLabel = 'DOAJ';
		parent::__construct($this->apiLabel);
	}

	public function apiUrl($searchTerm = '', $startAt = 0)
	{
		$searchTerm /* means ISSN here */ = implode(' ',array_map('trim',str_getcsv(preg_replace('/\s+/',' ',$searchTerm),' ')));	// 'anti-quotes' clean-up!(?)
		return ( 'https://www.doaj.org/api/search/journals/issn:' . $searchTerm );
		// for example: https://www.doaj.org/api/search/journals/issn:1210-5759
	}

	public function webUrl($searchTerm = '', $startAt = 0)
	{
		if ( !empty($searchTerm) ) {
			return ( 'https://www.doaj.org/toc/' . $searchTerm /* ISSN expected, e.g. https://www.doaj.org/toc/1802-8829 */ );
		}
		return 'https://www.doaj.org/';
	}
}
