<?php

class apiQueryIssnRomeo extends apiQueryUtilsIssn
{
	// see https://v2.sherpa.ac.uk/api/object-retrieval-by-id.html
	// Check HTML Output:
	// http://lib-cms-dev1/web/search.handler.php?req=json&api=JournalDetail&remote=romeo&issn=1436-5030,0065-3195&alma=99116742784005522&detail=publisher,issn,eissn,embargo-fund&target=lib4ri-journal-@-12&find=hydrogen
	
	public $returnAry = array();

	public function __construct()
	{
		$this->apiLabel = 'Sherpa/Romeo';
		$this->apiName = 'romeo';
		$this->apiKey = @strval( W_S_API_ROMEO_KEY );
		parent::__construct($this->apiLabel);
	}

	public function queryNow($issnList = '', $returnMode = 0 )
	{
		$this->setupIssn($issnList);

		foreach( $this->issnAry as $issnInput ) {

			$this->setApiUrl( 
				'https://v2.sherpa.ac.uk/cgi/retrieve_by_id?item-type=publication&api-key=' . $this->apiKey . '&format=Json&identifier=' . $issnInput
			);
			// for example: 
			// https://v2.sherpa.ac.uk/cgi/retrieve_by_id?item-type=publication&api-key=DDDD...4444&format=Json&identifier=1802-8829

			$this->setWebUrl( 
				'https://v2.sherpa.ac.uk/cgi/autocomplete/publication_title_abbreviation_and_issn?q=' . $issnInput /* works with ISSN too/directly */
			);

			$jsonCode = $this->makeQuery( $this->apiUrl );
			$jsonAry = $this->jsonDecode( $jsonCode, true );

			$gotSafeId = false;
			foreach( @array_slice($jsonAry['items'],0) as $jourData ) {
				if ( ( $id = @trim($jourData['id']) ) || ( $id = @trim($jourData['system_meta']['id']) ) ) {
					$iAry = array();
					foreach( @array_slice($jourData['issns'],0) as $issnData ) {
			/*			{
						   "type" : "print",
						   "issn" : "1993-2015"
						},
						{
						   "type" : "electronic",
						   "issn" : "1802-8829"
						}											*/
						if ( ( $issnFound = @trim($issnData['issn']) ) && intval($issnFound) ) {
							$tmp = substr($issnData['type'],0,1) /* to get 'p' or 'e' */ . 'Issn';
							$this->$tmp = $issnFound;
							$iAry[] = strtolower($issnFound);
						}
					}
					if ( in_array(strtolower($issnInput),$iAry) ) {
						$gotSafeId = true;
						$this->setWebUrl( 'https://v2.sherpa.ac.uk/id/publication/' . $id );
						$this->returnAry = array_merge_recursive( array($issnInput => $jourData), $this->returnAry );	// add it in front
						break;
					}
					if ( !$gotSafeId ) {
						$this->setWebUrl( 'https://v2.sherpa.ac.uk/id/publication/' . $id );
					}
				} 
				$this->returnAry = array_merge_recursive( $this->returnAry, array($issnInput => $jourData) );	// add it at the end
			}
			if ( $gotSafeId ) { break; }
		}
		
		if ( @!sizeof( array_filter( $this->returnAry ) ) ) {
			$this->returnAry = array('error' => 'ERROR: Nothing found at ' . $this->apiLabel . '!');
		}
		return $this->jsonOutput( $this->returnAry, $returnMode );
	}

}
