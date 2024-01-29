<?php

class apiQueryIssnDOAJ extends apiQueryUtilsIssn
{
	// see:	https://www.doaj.org/api/docs#
	//      https://www.doaj.org/api/docs#!/CRUD_Journals/get_api_journals_journal_id
	// web (text/title search): 
	//		https://www.doaj.org/search/journals?ref=homepage-box&source=%7B%22query%22%3A%7B%22query_string%22%3A%7B%22query%22%3A%22Entomology%22%2C%22default_operator%22%3A%22AND%22%7D%7D%2C%22track_total_hits%22%3Atrue%7D
	// api: https://www.doaj.org/api/search/journals/issn:1802-8829
	// 
	// Check HTML Output:
	// http://lib-cms-dev1/web/search.handler.php?req=json&api=JournalDetail&remote=doaj&issn=1423-0127,1021-7770&detail=publisher,issn,eissn,title&target=lib4ri-journal-@-1&find=hydrogen

	public $returnAry = array();
	protected $failedAry = array();

	public function __construct()
	{
		$this->apiLabel = 'DOAJ';
		$this->apiName = 'doaj';
		parent::__construct($this->apiLabel);
	}

	public function queryNow($issnList = '', $returnMode = 0 )
	{
		$this->setupIssn($issnList);
	
		foreach( $this->issnAry as $issnInput ) {

			$this->setApiUrl(
				'https://www.doaj.org/api/search/journals/issn:' . $issnInput
		/* 		for example:
				https://www.doaj.org/api/search/journals/issn:1210-5759		*/
			);
			$this->setWebUrl(
				'https://www.doaj.org/toc/' . $issnInput
		/*		for example:
				https://www.doaj.org/toc/1210-5759
				https://www.doaj.org/toc/07b6391abd76469f8b2a19376d0b6ddf		*/
			);

			$jsonCode = $this->makeQuery( $this->apiUrl );
			$jsonAry = $this->jsonDecode( $jsonCode, true );


			if ( !( $this->size($jsonAry['results']) || @intval($jsonAry['total']) ) ) {
				$this->failedAry[$issnInput] = $jsonAry;
				continue;
			}

			$issnConfirmed = false;
			foreach( @array_slice($jsonAry['results'],0) as $jourData ) {
				if ( $id = @trim($jourData['id']) ) {		// for example 07b6391abd76469f8b2a19376d0b6ddf
					$pIssnFound = @trim( $jourData['bibjson']['pissn'] );
					$eIssnFound = @trim( $jourData['bibjson']['eissn'] );
					if ( !empty($pIssnFound) && !empty($eIssnFound) ) {
				//		if ( strtolower($this->eIssn) != strtolower($pIssnFound) ) {
							if ( strtolower($issnInput) == strtolower($pIssnFound) ) {
								$issnConfirmed = true;
								$this->pIssn = $pIssnFound;
							} elseif ( @empty( $this->pIssn ) ) {
								$this->pIssn = $pIssnFound;
							}
				//		}
				//		if ( strtolower($this->pIssn) != strtolower($eIssnFound) ) {
							if ( strtolower($issnInput) == strtolower($eIssnFound) ) {
								$issnConfirmed = true;
								$this->eIssn = $eIssnFound;
							} elseif ( @empty( $this->eIssn ) ) {
								$this->eIssn = $eIssnFound;
							}
				//		}
					}
				} 
				$this->returnAry = array_merge_recursive( $this->returnAry, array($issnInput => $jourData) );	// add it at the end
			}
	//		if ( $issnConfirmed ) { break; }
		}
		
		if ( sizeof($this->failedAry) == sizeof($this->issnAry) ) {
			$this->returnAry = array('error' => 'ERROR: No journal found at ' . $this->apiLabel . '!', 'debug' => $this->failedAry );
		}
		elseif ( !$this->size( array_filter( $this->returnAry ) ) ) {
			$this->returnAry = array('error' => 'ERROR: Nothing found at ' . $this->apiLabel . '!');
		}
		return $this->jsonOutput( $this->returnAry, $returnMode );
	}
}
