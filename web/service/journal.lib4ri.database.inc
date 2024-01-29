<?php

class apiQueryIssnApi4RI extends apiQueryUtilsIssn
{
	// For own Lib4RI services, no documentation (yet), see code comments: https://github.com/Lib4RI/lib4ri-websearch/blob/master/api/index.php

	public function __construct()
	{
		$this->apiLabel = 'Api4RI';
		parent::__construct($this->apiLabel);
	}

	public function apiUrl($searchTerm = '', $startAt = 0)
	{
		$searchTerm /* means ISSN here */ = implode(' ',array_map('trim',str_getcsv(preg_replace('/\s+/',' ',$searchTerm),' ')));	// 'anti-quotes' clean-up!(?)
		return ( rtrim( W_S_API_LIB4RI_URL, '/' ) . '/?issn=' . $searchTerm );
		// for example: http://lib-cms-dev1/web/api/?issn=1475-939X,1432-5233
	}

	public function webUrl($searchTerm = '', $startAt = 0)
	{
		return 'https://www.lib4ri.ch/resources/journals.html';		// aux/temp.
	}
}
