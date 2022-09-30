<?php

@include_once('./search.config.inc');


// ==================================================================================

class apiQueryUtilsIssn extends apiQueryUtils
{
	// remind, $searchTerm may (also) be the (e)ISSN
	public $issnAry;	// ISSN(s) of the journal we are insterested in
	public $pIssn;		// for (print) issn *found*
	public $eIssn;		// for electronical issn *found*

	public function __construct($apiLabel)
	{
		parent::__construct($apiLabel);
		$this->setupParam();
		$this->setupIssn();
	}
	
	public function setupIssn($issnList = '')
	{
		if ( ( $tmp = trim($issnList) ) || ( $tmp = @trim(strip_tags($_GET['issn'])) ) ) {
			$this->issnAry = array_map('trim',explode(',',strtr($tmp,';',',')));
			return;
		}
		$issnList = $this->cleanIssn($this->searchTerm);	// assumption
		if ( @strpos($issnList,'-') ) {		// rough check, so not empty
			$ary = array();
			preg_match_all('/\d{4}\s*\-\s*\d{4}|\d{4}\s*\-\s*\d{3}X/',$issnList,$ary);
			$this->issnAry = array_map( array($this,'cleanIssn'), array_values($ary[0]) );
		}
	}
}


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


			if ( !( @sizeof($jsonAry['results']) || @intval($jsonAry['total'] ) ) ) {
		//		$this->exitByError( 'ERROR: got no Journal from DOAJ !?' );
		//		$this->returnAry = array('error' => 'ERROR: No journal found at ' . $this->apiLabel . '!');
				$jsonAry['error'] = 'ERROR: No journal found at ' . $this->apiLabel . '!';
				$this->returnAry = $jsonAry;
				return $this->jsonOutput( $this->returnAry, $returnMode );
			}

			$issnConfirmed = false;
			foreach( @array_slice($jsonAry['results'],0) as $jourData ) {
				if ( $id = @trim($jourData['id']) ) {		// for example 07b6391abd76469f8b2a19376d0b6ddf
					$issnFound = @trim( $jourData['bibjson']['pissn'] );
					if ( strtolower($issnInput) == strtolower($issnFound) ) {
						$issnConfirmed = true;
						$this->pIssn = $issnFound;
					} elseif ( @empty( $this->pIssn ) ) {
						$this->pIssn = $issnFound;
					}
					$issnFound = @trim( $jourData['bibjson']['eissn'] );
					if ( strtolower($issnInput) == strtolower($issnFound) ) {
						$issnConfirmed = true;
						$this->eIssn = $issnFound;
					} elseif ( @empty( $this->eIssn ) ) {
						$this->eIssn = $issnFound;
					}
				} 
				$this->returnAry = array_merge_recursive( $this->returnAry, array($issnInput => $jourData) );	// add it at the end
			}
			if ( $issnConfirmed ) { break; }
		}
		
		if ( @!sizeof( array_filter( $this->returnAry ) ) ) {
			$this->returnAry = array('error' => 'ERROR: Nothing found at ' . $this->apiLabel . '!');
		}
		return $this->jsonOutput( $this->returnAry, $returnMode );
	}
}



class apiQueryJournalDetail extends apiQueryUtilsIssn
{
	// Example:
	// http://lib-cms-dev1/web/search.handler.php?req=json&api=JournalDetail&remote=swisscovery&issn=1990-1356&alma=99116725724505522&detail=available,linklist&target=lib4ri-journal-@-7&find=hydrogen
	
	// remind, $searchTerm may (also) be the (e)ISSN
	public $apiRemote = '';
	public $htmlTarget;		// id of HTML element like 'lib4ri-journal-@-12345', can have a @-placeholder to insert detail-type
	public $almaId = '';
	public $detailAry;
	public $returnAry = array();
	public $ccLinkAry = array(
		'cc' => 'https://creativecommons.org/licenses/',
		'cc-by' => 'https://creativecommons.org/licenses/by/4.0/',
		'cc-by-nc' => 'https://creativecommons.org/licenses/by-nc/4.0/',
		'cc-by-nd' => 'https://creativecommons.org/licenses/by-nd/4.0/',
		'cc-by-nc-nd' => 'https://creativecommons.org/licenses/by-nc-nd/4.0/',
		'cc-by-nd-nc' => 'https://creativecommons.org/licenses/by-nc-nd/4.0/',
	);


	public function __construct($apiRemote = '',$issnList = '')
	{
		$this->apiName = 'journaldetail';		// lower-case!
		$this->apiLabel = 'Journal Detail';
		parent::__construct($this->apiLabel);

		$tmp = trim($apiRemote);
		$this->apiRemote = ( empty($tmp) ? @trim(strip_tags($_GET['remote'])) : $tmp );
		$this->htmlTarget = @trim(strip_tags($_GET['target']));

		if ( @empty($this->almaId) ) {
			if ( $tmp = @trim(strip_tags($_GET['alma'])) ) {
				$this->almaId = ( intval($tmp) ? 'alma'.$tmp : $tmp );		// to get alma99867474...
			}
		}
		if ( @!is_array($this->detailAry) || empty($this->detailAry) ) {
			if ( $tmp = @strtolower(strip_tags($_GET['detail'])) ) {
				$this->detailAry = array_map('trim',explode(',',strtr($tmp,';',',')));
			}
		}

		$this->setupParam();
	}

	// function to compose and update the array (to be returned as json to lib4ri-bentobox.js)
	// with HTML code/markup. The array keys are the HTML element IDs where to place it.
	// Note: Usually 'action' is a number from 0-9 to assign a replacement priority, however it also can be
	// any character to replace anything afterwards, or '^' to add it in front or '~' to add it at the end.
	private function updateReturnHtml($detailName,$markup = '',$action = '0',$okEmpty = false)
	{
		if ( !$okEmpty && empty(trim($markup)) ) {
			return;		// No HTML markup got, and not allowed, hence return!
		}
		$tagWeight = '';	// something like <!-- prio:3
		if ( $weight = intval($action) ) {
			$tagWeight = '<!-- prio:' . min($weight,9) . ' -->';
		} elseif ( !empty($action) ) {
			$tagWeight = '<!-- prio:' . substr($action,0,1) . ' -->';	// expection only one character
		}
		if ( @empty($this->htmlTarget) ) {
			if ( !empty($tagWeight) ) {
				$tmp = @strval($this->returnAry['html']);
				$this->returnAry['html'] = $tagWeight . ( ( substr($tmp,0,10) == '<!-- prio:' ) ? substr(strchr($tmp,'>'),1) : $tmp ) . $markup;
			} else {
				$this->returnAry['html'] = $markup;
			}
		} else { // with HTML target
			$htmlId = str_replace('@',$detailName,$this->htmlTarget);		// id of HTML element
			$tmpAry = array( 'html' => array( $htmlId => $tagWeight.$markup ) );
			if ( @is_array($this->returnAry) ) {
				$this->returnAry = array_merge_recursive( $this->returnAry, $tmpAry );
			} else {
				$this->returnAry = $tmpAry;
			}
		}
	}

	private function markupIssn( $issn = '0000-000X', $label = 'ISSN' )
	{
		return ( '<span class="lib4ri-tabul-issn">' . $label . '</span>: &nbsp;' . $issn );
	}
	
	// getDetail() - Test Links:
	// incoming:
	//		http://lib-cms-dev1/web/search.handler.php?req=json&api=JournalDetail&detail=available&find=Entomology&ident=9031747386565056743
	//		http://lib-cms-dev1/web/search.handler.php?req=json&api=JournalDetail&detail=available&find=Hydrogen&ident=9031747386565056743	
	// outgoing:
	//		api: https://lib4ri.swisscovery.slsp.ch/primaws/rest/pub/pnxs?blendFacetsSeparately=false&disableCache=false&getMore=0&inst=41SLSP_LIB&pcAvailability=false&newspapersActive=false&newspapersSearch=false&offset=0&lang=en&limit=987654321&q=any,contains,Entomology&qExclude=&qInclude=facet_frbrgroupid,exact,9081658851379871370&rapido=false&refEntryActive=true&rtaLinks=true&scope=DN_and_CI&searchInFulltextUserSelection=true&skipDelivery=Y&sort=date_d&tab=41SLSP_LIB_DN_CI&vid=41SLSP_LIB:GO
	//		web: see code
	public function getUrl($detailList = '', $searchTerm = '', $searchIdent = '', $almaId = '')		/* OBSOLETE !? */
	{
		$detailList = ( empty($detailList) ? implode(',',$this->detailAry) : $this->cleanTerm($detailList) );
		$searchTermEnc = rawurlencode( empty($searchTerm) ? ( empty($this->searchTermOrig) ? setupParam('searchTermOrig') : $this->searchTermOrig ) : $searchTerm );
	}

	public function apiUrl( $data = null, $startAt = 0 )
	{
		return ( $this->apiUrl );
	}
	public function webUrl( $data = null, $startAt = 0 )
	{
		return ( $this->webUrl );
	}

	public function exchangeRate( $currency = '$', $curGoal = 'CHF' )
	{
		if ( $currency == $curGoal ) {
			return 1.0;		// special case shortcut, since we are only interested in the sheer floating number (and not all exchange rate data)
		}
		$apiTool = new apiQueryCurrency( 'float' );
		$exData = $apiTool->queryNow( ( strpos($currency,'/') ? $currency : $currency.'/'.$curGoal ) );
		if ( is_array($exData) ) {
			$reqTerm = array_keys($exData)[0];
			return $exData[$reqTerm][0];
		}
		return $exData;
	}

	public function exchangeFormat( $cur = '', $value = 0.0, $exRate = 1.0, $curGoal = 'CHF' )
	{
		$tmp = ( $cur != $curGoal ) ? ('About '. $curGoal . ' ' . round($value*$exRate) . '.--') : '';
		return '<span title="' . $tmp . '">' . rtrim( strval($value) . ' ' . $cur ) . '</span>';
	}

	public function askDoaj()
	{
		// Not OA: https://www.doaj.org/api/search/journals/issn:2589-3114
		// Found : https://www.doaj.org/api/search/journals/issn:1210-5759
		//         https://www.doaj.org/api/search/journals/07b6391abd76469f8b2a19376d0b6ddf

		$apiPrio = 6;
		$apiAux = new apiQueryIssnDOAJ();
		$jsonAry = $apiAux->queryNow('','array'); // already inside 'results' (check link above), key will be the ISSN, and the 1st should be the best found

		if ( @isset($jsonAry['error']) && !empty($jsonAry['error']) ) { return; }
		if ( @isset($jsonAry['total']) && !intval($jsonAry['total']) ) { return; }
		$jourData = reset( $jsonAry );	// assuming the first is the sole or at least the best result (should there be multible)

		$detailName = 'issn';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( $markup = @trim($apiAux->pIssn) ) {							// assigned by queryNow()
				$markup = $this->markupIssn( $markup );
				$this->updateReturnHtml($detailName,$markup,$apiPrio);
			}
		}
		$detailName = 'eissn';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( $markup = @trim($apiAux->eIssn) ) {							// assigned by queryNow()
				$markup = $this->markupIssn( $markup, 'E-ISSN' );
				$this->updateReturnHtml($detailName,$markup,$apiPrio);
			}
		}

		$detailName = 'linklist';
	//	if ( in_array($detailName,$this->detailAry) ) { // add generally:
			if ( ( $issn = @trim($apiAux->eIssn) ) || ( $issn = @trim($apiAux->pIssn) ) ) {
				$markup = 'https://www.doaj.org/toc/' . $issn;
				$markup = ' | <a href="' . $markup . '" target="_blank">DOAJ</a>';
				$this->updateReturnHtml($detailName,$markup,'~');	// '~' to attach it
			}
	//	}

		$detailName = 'title';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( $markup = @trim( $jourData['bibjson']['title'] ) ) {
				if ( W_S_RESULT_TITLE_LINKED && ( $tmp = @trim( $jourData['bibjson']['ref']['journal'] ) ) ) {
					$markup = '<a href="' . $tmp . '" target="_blank">' . $markup . '</a>';
				}
				$this->updateReturnHtml($detailName,$markup,$apiPrio);
			}
		}

		$detailName = 'publisher';
		if ( in_array($detailName,$this->detailAry) ) {
			$pubAry = [];
			if ( $markup = @trim( $jourData['bibjson']['publisher']['name'] ) ) {
				if ( $loc = @trim( $jourData['bibjson']['publisher']['country'] ) ) {
					 $loc = $markup . ' (' . $loc . ')';
				}
		/* 2022-06-23: link currently not wanted
				if ( $tmp = @trim( $jourData['bibjson']['ref']['oa_statement'] ) ) {	// used as publisher link - there is no better...
					$markup = '<a href="' . $tmp . '" target="_blank" title="' . $loc . '">' . $markup . '</a>';
				} else {
					$markup = '<span title="' . $loc . '">' . $markup . '</span>';
				}
		*/
				$pubAry[] = '<span title="' . $loc . '">' . $markup . '</span>';
			}
		/* Not wanted according to Lothar 2022-09-22:
			if ( $markup = @trim( $jourData['bibjson']['institution']['name'] ) ) {		// Society/Institution, see 1953-8189 or 'Parasite' 1252-607X 
				if ( $loc = @trim( $jourData['bibjson']['institution']['country'] ) ) {
					$loc = $markup . ' (' . $loc . ')';
				}
				$pubAry[] = '<span title="' . $loc . '">' . $markup . '</span>';
			}
		*/
			if ( sizeof($pubAry) ) {
				if ( sizeof($pubAry) < 2 ) {
					$markup = 'Publisher: ' . @strval($pubAry[0]);
				} else {
					$markup = 'Publishers: <ul class="lib4ri-ul-flat"><li class="lib4ri-li-wide">' . implode('</li><li class="lib4ri-li-wide">',$pubAry) . '</li></ul>';
				}
				$this->updateReturnHtml($detailName,$markup,$apiPrio);
			}
		}

		$detailName = 'license';
		if ( in_array($detailName,$this->detailAry) ) {
			$ary = array();
			foreach( $jourData['bibjson']['license'] as $idx => $lic ) {		// there can be multiple once, see: https://www.doaj.org/api/search/journals/issn:1423-0127
				$idx = preg_replace('/\s+/',' ',trim(strtr(html_entity_decode($lic['type']),'-',' ')));
				if ( !( $url = @rtrim($lic['url']) ) ) {
					$url = @strval( $this->ccLinkAry[strtolower($idx)] );
				}
				$ary[strtoupper($idx)] = ( @empty($url) ? $lic['type'] : ( '<a href="' . $url . '" target="_blank">' . $lic['type'] . '</a>' ) );
			}
			if ( sizeof($ary) > 1 ) {
				$tmp = 'Recommended by Lib4RI';		// for CC BY we add 'recommended'
				foreach( $ary as $idx => $lic ) {
					if ( $idx != 'CC BY' ) { continue; }

					if ( substr($lic,0,3) == '<a ' ) {
						$ary[$idx] = substr($lic,0,3) . 'title="' . $tmp . '"' . substr($lic,2) . ' <span title="' . $tmp . '"> <span class="lib4ri-recommended">recommended</span></span>';
					} else {
						$ary[$idx] = '<span title="' . $tmp . '">' . $lic . ' <span class="lib4ri-recommended">recommended</span></span>';
					}
				}
			}
			if ( sizeof($ary) ) {
				$markup = implode(' or ',$ary) . ' license' /* . ( sizeof($ary) == 1 ? '' : 's' ) */ ;
			} else {
				$markup = '<!-- No license information available -->';
			}
			$this->updateReturnHtml($detailName,$markup,$apiPrio);
		}

		$detailName = 'agreement';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( $tmp = @trim( $jourData['id']) ) {
				$markup = 'Gold Open Access Journal';
			//	$markup = '<a href="' . 'https://www.doaj.org/toc/' . ( empty($apiAux->eIssn) ? $apiAux->pIssn : $apiAux->eIssn ) . '" target="_blank">' . $markup . '</a>';
				$markup .= ' (<a href="' . 'https://www.doaj.org/toc/' . ( empty($apiAux->eIssn) ? $apiAux->pIssn : $apiAux->eIssn ) . '" target="_blank">details</a>)';
				$this->updateReturnHtml($detailName,$markup,$apiPrio);
			}
		}

		$detailName = 'reviewed';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( isset($jourData['bibjson']['editorial']['review_process']) ) {
				$markup = implode(' + ',$jourData['bibjson']['editorial']['review_process']);
				$this->updateReturnHtml($detailName,$markup,$apiPrio);
			}
		}

		$detailName = 'embargo-fund';
		if ( in_array($detailName,$this->detailAry) ) {
			// reset agreement info:
			// $this->updateReturnHtml('agreement','<!-- reset -->',8);
			$markup = 'Gold Open Access Journal';
		//	$markup = '<a href="' . 'https://www.doaj.org/toc/' . ( empty($apiAux->eIssn) ? $apiAux->pIssn : $apiAux->eIssn ) . '" target="_blank">' . $markup . '</a>';
			$markup .= ' (<a href="' . 'https://www.doaj.org/toc/' . ( empty($apiAux->eIssn) ? $apiAux->pIssn : $apiAux->eIssn ) . '" target="_blank">details</a>)';
			$limitAry = array( 2300, 2600 );		// to be made configurable!? Also '2500' mentioned below!?
			$chfAry = array();
			$detAry = array( 'APC status unknown' );
			$tmp = strtolower( @trim($jourData['bibjson']['apc']['has_apc']) );	// can be 'no', 'none', 'true', 'yes' OR evaluated false
			if ( empty($tmp) && @isset($jourData['bibjson']['apc']) ) {
				$detAry[0] = 'No APC required';
			} elseif ( @substr($tmp,0,1) == 'n' || @substr($tmp,0,5) == 'false' || ( $tmp != '' && @substr($tmp,0,1) == '0' ) ) {
				$detAry[0] = 'No APC required';
			}
			elseif ( $tmp = @sizeof($jourData['bibjson']['apc']) ) {
				$detAry[0] = 'APC required';
				if ( $tmp === 1 && @isset($jourData['bibjson']['apc']['max']) ) {
					// special case, the 'max' index implies a range!?
					$cur = @trim( $jourData['bibjson']['apc']['max'][0]['currency'] );
					$val = @intval( $jourData['bibjson']['apc']['max'][0]['price'] );
					$exRate = @floatval( $this->exchangeRate( $cur ) );
					$chfAry[0] = round($val * $exRate);
					$detAry[0] .= ', up to ' . $this->exchangeFormat( $cur, $val, $exRate );
				} elseif ( $tmp > 0 ) {	// for a single price or a price range like '1250-2500 EUR'
					$apcAry = array( /* currency => array( price1, price2, ... ) */ );
					// we have to deal with e.g.
					// - $jourData['bibjson']['apc']['max'][0]['price']
					// - $jourData['bibjson']['apc']['max'][0]['currency']
					foreach( $jourData['bibjson']['apc'] as $mIdx => $mAry ) {
						foreach( $mAry as $ary ) {
							if ( $tmp = intval($ary['price']) ) {
								$apcAry[trim($ary['currency'])][] = $tmp;
							}
						}
					}
					foreach( $apcAry as $cur => $ary ) {
						if ( !empty($cur) /* = currency */ ) {
							sort($ary);
							$exRate = @floatval( $this->exchangeRate( $cur ) );
							$chfAry[0] = round( $ary[0] * $exRate);
							if ( sizeof($ary) < 2 ) {
								$detAry[0] .= ': up to ' . $this->exchangeFormat( $cur, $ary[0], $exRate );
							} else {
								$tmp = $this->exchangeFormat( '', $ary[0], $exRate );
								$val = @intval( array_pop($ary) );
								$chfAry[1] = round( $val * $exRate );
								$detAry[0] .= ': ' . $tmp . '-' . $this->exchangeFormat( '', $val, $exRate ) . ' ' . $cur;
							}
							break;
						}
					}
				}
			}
			if ( $tmp = @trim( $jourData['bibjson']['apc']['url']) ) {
				$detAry[0] .= ' (<a href="' . $tmp . '" target="_blank">details</a>)';
			}
		//	else { $detAry[0] = rtrim( $detAry[0], ".,;: \r\t\n\v\x00"); }

			if ( sizeof($chfAry) ) {
				$val = @intval($chfAry[1]);
				if ( ( $val && $val < $limitAry[1] ) || ( $chfAry[0] < $limitAry[1] ) ) {
					$tmp = '<a href="https://www.lib4ri.ch/services/open-access/open-access-fund.html" target="_blank">' . 'check Lib4RI Fund' . '</a>';
					if ( !empty($val) || $chfAry[0] > $limitAry[0] ) {
						$tmp .= ' (if &lt; CHF 2500)';
					}
					$detAry[] = $tmp;
				}
			}

			$ary = [];
			foreach( $jourData['bibjson']['license'] as $idx => $lic ) {		// there can be multiple once, see: https://www.doaj.org/api/search/journals/issn:1423-0127
				$idx = preg_replace('/\s+/',' ',trim(strtr(html_entity_decode($lic['type']),'-',' ')));
				$url = '';
		/*		if ( !( $url = @rtrim($lic['url']) ) ) {		// 2022-06-23: URLs not wanted
					$url = @strval( $this->ccLinkAry[strtolower($idx)] );
				}		*/
				$ary[strtoupper($idx)] = ( @empty($url) ? $lic['type'] : ( '<a href="' . $url . '" target="_blank">' . $lic['type'] . '</a>' ) );
			}
			if ( sizeof($ary) > 1 ) {
				$tmp = 'Recommended by Lib4RI';		// for CC BY we add 'recommended'
				foreach( $ary as $idx => $lic ) {
					if ( $idx != 'CC BY' ) { continue; }
		/*
					if ( substr($lic,0,3) == '<a ' ) {
						$ary[$idx] = substr($lic,0,3) . 'title="' . $tmp . '"' . substr($lic,2) . ' <span title="' . $tmp . '"><span class="lib4ri-recommended">recommended</span></span>';
					} else {
						$ary[$idx] = '<span title="' . $tmp . '">' . $lic . ' <span class="lib4ri-recommended">recommended</span></span>';
					}
		*/
					$ary[$idx] = $lic . ' (<a href="https://oaspa.org/why-cc-by/" target="_blank" title="' . $tmp . '"><span class="lib4ri-recommended">recommended</span></a>)';
				}
			}
			if ( sizeof($ary) ) {
				$detAry[] = implode(' or ',$ary) . ' license' /* . ( sizeof($ary) == 1 ? '' : 's' ) */ ;
			}
			$cssAux = 'style="position:relative; top:' . ( empty(trim(strip_tags($markup))) ? '-0.1' : '0.1' ) . 'em;"'; // not happy about this... - TO BE TUNED
			$markup .= ' <ul class="lib4ri-ul-flat" ' . $cssAux . '><li class="lib4ri-li-wide">' . implode('</li><li class="lib4ri-li-wide">',$detAry) . '</li></ul><br />';
			$this->updateReturnHtml($detailName,$markup,$apiPrio);
		}
	}

	// for Sherpa/Romeo: helper function returning true/false if content should be ignored/shown
	protected function askRomeoIgnored( $policyData )
	{
		// only show without OA fee (Jochen, Lothar):
		if ( @stripos($policyData['additional_oa_fee'],'no') === false ) { return true; }

		// ignore submitted versions (Lothar):
		$tmp = @strval( $policyData['article_version_phrases'][0]['value'] );	// published, accepted, ...
		if ( !empty($tmp) && stripos($tmp,'submit') !== false ) { return true; }

		// Show the document version only if it can be published on 'freely accessible' places:
		$ary = array_map( 'strtolower', $policyData['location']['location'] );
		foreach( $ary as $platform ) {
			//	keep this flexible, there are also term like 'institutional_website' or 'non_commercial_institutional_repository':
			if ( strpos($platform,'institutional_') !== false ) { /* ok, so not ignored */ return false; }
			if ( strpos($platform,'non_commercial') !== false ) { /* ok, so not ignored */ return false; }
			if ( strchr($platform,'any_repo') || strchr($platform,'any_web') || strchr($platform,'any_lib') ) { /* ok, so not ignored */ return false; }
		}
		return true;	// true for ignoring it
	}

	public function askRomeo()
	{
		// https://v2.sherpa.ac.uk/cgi/retrieve_by_id?item-type=publication&api-key=DDDD...4444&format=Json&identifier=1045-6767
		// https://v2.sherpa.ac.uk/cgi/retrieve_by_id?item-type=publication&api-key=DDDD...4444&format=Json&identifier=1802-8829

	//	$apiPrio = 2;	// not used, here means data will be filled in by lib4ri-bentobox.js if no (higher) priority is assinged already
		$apiAux = new apiQueryIssnRomeo();
		$jsonAry = $apiAux->queryNow('','array'); // already inside 'items' (check link above), and the 1st should be the best found
		$jourData = ( @!empty($jsonAry['error']) ? array() : reset( $jsonAry ) /* it's always the first one!(?) */ );

		// Sometimes ISSNs are also messed up with Sherpa/Romeo!
		$detailName = 'issn';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( ( @!empty($apiAux->pIssn) && @!empty($apiAux->eIssn) ) || !in_array('eissn',$this->detailAry) ) {
				$markup = $this->markupIssn( @trim($apiAux->pIssn) );			// assigned by queryNow()
				$this->updateReturnHtml($detailName,$markup,':');		// to add after ':'
			}
		}
		$detailName = 'eissn';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( ( @!empty($apiAux->pIssn) && @!empty($apiAux->eIssn) ) || !in_array('issn',$this->detailAry) ) {
				$markup = $this->markupIssn( @trim($apiAux->eIssn), 'E-ISSN' );			// assigned by queryNow()
				$this->updateReturnHtml($detailName,$markup,':');		// to add after ':'
			}
		}

		$detailName = 'publisher';
		if ( in_array($detailName,$this->detailAry) && @sizeof($jourData['publishers']) ) {
			$pubAry = [];
			foreach( $jourData['publishers'] as $ary ) {
				if ( !@empty($ary['relationship_type']) && sizeof($jourData['publishers']) > 1 && !stristr( @strval($ary['relationship_type']), 'publish' ) ) {
					// we only want to show (commercial) publishers (aside of associations and imprint) supposed there are explicitly
					continue;
				}
				if ( $markup = @trim( $ary['publisher']['name'][0]['name']) ) {
					if ( $loc = @trim( $ary['publisher']['country_phrases'][0]['phrase'] ) ) {
						$loc = $markup . ' (' . $loc . ')';	// adding country name
					} else {
						$loc = $markup;
					}
					// 2022-06-23: link currently not wanted
			/*		if ( $tmp = @trim( $ary['publisher']['url'] ) ) {
						$markup = '<a href="' . $tmp . '" target="_blank" title="' . $loc . '">' . $markup . '</a>';
					} else {
						$markup = '<span title="' . $loc . '">' . $markup . '</span>';
					}		*/
					$pubAry[] = '<span title="' . $loc . '">' . $markup . '</span>';
				}
			}
			if ( sizeof($pubAry) ) {
				if ( sizeof($pubAry) < 2 ) {
					$markup = 'Publisher: ' . @strval($pubAry[0]);
				} else {
					$markup = 'Publishers: <ul class="lib4ri-ul-flat"><li class="lib4ri-li-wide">' . implode('</li><li class="lib4ri-li-wide">',$pubAry) . '</li></ul>';
				}
				$this->updateReturnHtml($detailName,$markup /* , $apiPrio */ );
			}
		}

		$detailName = 'title';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( $markup = @trim( $jourData['title'][0]['title'] ) ) {
				if ( W_S_RESULT_TITLE_LINKED && ( $tmp = @trim( $jourData['url'] ) ) ) {
					$markup = '<a href="' . $tmp . '" target="_blank">' . $markup . '</a>';
				}
				$this->updateReturnHtml($detailName,$markup /* , $apiPrio */ );
			}
		}

		$detailName = 'license';
		if ( in_array($detailName,$this->detailAry) && @!empty($jourData['publisher_policy']) ) {
			$licAry = [];
			$licUri = '';
			foreach( $jourData['publisher_policy'] as $pubPolicy ) {
				foreach( $pubPolicy["permitted_oa"] as $pData ) {

					if ( $this->askRomeoIgnored( $pData ) ) { continue; }

					foreach( $pData['license'] as $pIdx => $lic ) {
						foreach( $lic['license_phrases'] as $ary ) {
							$licAry[strtolower($ary['phrase'])] = $ary['phrase'];
							if ( empty($licUri) ) {
								$licUri = @trim($pubPolicy['permitted_oa']['uri']);
							}
						}
					}
				}
			}
			if ( sizeof($licAry) ) {
				$markup = implode(', ',$licAry);
				if ( !empty($licUri) ) {
					$markup = '<a href="' . $licUri . '" target="_blank">' . $markup . '</a>';
				}
				$this->updateReturnHtml($detailName,'License: '.$markup /* , $apiPrio */ );
			}
		}

		$detailName = 'agreement';
		if ( in_array($detailName,$this->detailAry) ) {
			if ( @!empty($jourData['publisher_policy']) ) {
				$markup = 'Green Open Access';
			//	$url = @trim($jourData['publisher_policy'][0]['uri']);	// get the link to publishers policy
				$issn = trim( @empty($apiAux->eIssn) ? $apiAux->pIssn : $apiAux->eIssn );
				$url = /* $apiAux->webUrl() does not exist yet */ 'https://v2.sherpa.ac.uk/cgi/romeosearch?publication_title-auto=' . $issn;
			//	$markup = '<a href="' . $url . '" target="_blank">' . $markup . '</a>';
				$markup .= ' (<a href="' . $url . '" target="_blank">details</a>)';
				$this->updateReturnHtml($detailName,$markup /* , $apiPrio */ );
			}
		}

		$detailName = 'embargo-fund';
		if ( in_array($detailName,$this->detailAry) ) {
			$markup = '<!-- Green Open Access -->';
			$oaGreenAry = array();
			if ( @sizeof($jourData['publisher_policy']) ) {
				$markup = 'Green Open Access';
				foreach( $jourData['publisher_policy'] as $pubIdx => $pubPolicy ) {
					foreach( $pubPolicy["permitted_oa"] as $pIdx => $pData ) {

						if ( $this->askRomeoIgnored( $pData ) ) { continue; }

						// 2022-06-23: Do not show funder related prerequisites:
						if ( @sizeof($pData['prerequisites']) === 1 ) {
							$ary = array_keys( $pData['prerequisites'] );
					//		if ( stripos($ary[0],'fund') !== false ) { continue; }	/* re-enabled 2022-07-29 */
						}

						// Version (plus License + Pre-Requsites):
						$tmp = $pData["article_version_phrases"][0]["phrase"] . ' Version';
						$lic = '';	// License Data
				//		foreach( $pData["license"][0]["license_phrases"] as $ary ) {
				//			$lic .= ( empty($lic) ? 'License: ' : ', ' ) . $ary['phrase'];
				//		}
						if ( @sizeof($pData['conditions']) ) { // add explicit conditions to the license
							$lic .=  ( empty($lic) ? '' : "\r\n" ) . 'Condition' . ( sizeof($pData['conditions']) == 1 ? '' : 's' ) . ":\r\n" . implode("\r\n", array_map( function($h) { return '- '.$h; },$pData['conditions'] ) );
						}
						if ( !( $url = @rtrim($pubPolicy['permitted_oa']['uri']) ) && !empty($lic) ) {
							$idx = preg_replace('/\s+/',' ',trim(strtr(html_entity_decode($lic),'-',' ')));
							$url = @strval( $this->ccLinkAry[strtolower($idx)] );
						}
				/*
						if ( @!empty($url) ) {
							$tmp = '<a href="' . $url . '" target="_blank"' . ( empty($lic) ? '>' : ' title="License: ' . $lic . '">' ) . $tmp . '</a>';
						} elseif ( !empty($lic) ) {
							$tmp = '<span title="' . $lic . '">' . $tmp . '</span>';
						}
				*/
						$tmp = '<span title="' . $lic . '">' . $tmp . '</span>';
						if ( @!empty($url) ) {		// 2022-06-23: Link separate!
							$tmp .= ' (<a href="' . $url . '" target="_blank">details</a>)';
						}

						if ( @isset($pData['prerequisites']) ) {		// to store pre-requisites, check 1474-4457
							$ary = [];
							// take care, there are all kind of data and data structres!
							foreach( $pData['prerequisites'] as $reqIdx => $reqAry ) {
								if ( $reqIdx == 'prerequisites_phrases' ) {
									foreach( $reqAry as $rAry ) {
										$ary[] = $rAry['phrase'];
									}
								} elseif ( substr($reqIdx,0,13) == 'prerequisite_' ) {
									// let's interpret the index, since there may be a diffuse data structure underneath
									$ary[] = ucwords(strtr(substr($reqIdx,13),'_',' '));
								}
							}
							$tmp .= ( @empty($ary) ? ':' : ': <span title="Prerequisite' . ( sizeof($ary) < 2 ? ( sizeof($ary) ? ': ' : '!?' ) : "s:\r\n" ) . implode("\r\n",$ary) . '" style="color:#f71;">&#9888;</span>' );
						}
						$htmlAry = array( $tmp );

						$tmp = '';	// Repository type:
						foreach( $pData["location"]["location_phrases"]  as $ary ) {
							if ( substr($tmp,-10) == 'Repository' && strpos($ary['phrase'],'Repository') ) {
								$tmp = rtrim(substr($tmp,0,-10)) . '/' . $ary['phrase'];
							} else {
								$tmp .= ( empty($tmp) ? '' : ', ' ) . $ary['phrase'];
							}
						}
						$htmlAry[] = $tmp;
		
						$tmp = ''; // Embargo Period:
						if ( @intval($pData["embargo"]["amount"]) ) {
							$tmp = 'Embargo: ' . $pData["embargo"]["amount"] . ' ' . @strval($pData["embargo"]["units_phrases"][0]["phrase"]);
						} else {
							$tmp = 'Embargo: None';
						}
						$htmlAry[] = $tmp;

						$lic = '';	// License Data
						foreach( $pData["license"][0]["license_phrases"] as $ary ) {
							$lic .= ( empty($lic) ? '' : ', ' ) . $ary['phrase'];
						}
						if ( !empty($lic) ) {
							$htmlAry[] = 'License' . ( strpos($lic,',') ? 's: ' : ': ' ) . $lic;
						}

						$tmp = array_shift( $htmlAry );		// 1st is the line with Accepted/Published version
						$htmlAry = array_map( function($h) { return '<li class="lib4ri-li-wide">'.$h.'</li>'; }, $htmlAry );
						$tmp .= '<ul class="lib4ri-ul-flat">' . implode("\r\n",$htmlAry) . '</ul>';
						$oaGreenAry[] = $tmp;
					}
				}
			}
			$no_info = empty( trim( @strip_tags($markup) ) );
			if ( !$no_info || !empty($oaGreenAry) ) {
				$issn = trim( @empty($apiAux->eIssn) ? $apiAux->pIssn : $apiAux->eIssn );
				$url = /* $apiAux->webUrl() does not exist yet */ 'https://v2.sherpa.ac.uk/cgi/romeosearch?publication_title-auto=' . $issn;
				$markup .= ' (<a href="' . $url . '" target="_blank">details</a>)';
			}
			if ( !empty($oaGreenAry) ) {
				$cssAux = 'style="position:relative; top:' . ( $no_info ? '-0.1' : '0.1' ) . 'em;"'; // not happy about this... - TO BE TUNED
				$oaGreenAry = array_map( function($h) { return '<li class="lib4ri-li-wide">'.$h.'</li>'; }, $oaGreenAry );
				$markup .= ' <ul class="lib4ri-ul-flat" ' . $cssAux . '>' . implode('',$oaGreenAry) . '</ul><br />';
			}
			$this->updateReturnHtml($detailName,$markup /* , $apiPrio */ );
		}
	}

	protected function askApi4riHelper($issnList = null,$wantedList = null)
	{	
		// function to switch automatically to our web API if local libraries cannot found.
		// An array will be returned (see api.classes.inc for example/structure).
		if ( ( $dir = @rtrim( W_S_API_LIB4RI_DIR, '/' ) ) && @include_once($dir.'/api.classes.inc') ) {
			$api4ri = new get4riJournal($issnList,$wantedList);
			return $api4ri->getResult(); // as array
		}
		$apiUrl = rtrim( W_S_API_LIB4RI_URL, '/' ) . '/?issn=' . $issnList . '&wanted=' . $wantedList;
		$apiTool = new apiQueryUtils( 'JIF' /* any unused name */ );
		$jsonCode = $apiTool->makeQuery( $apiUrl );
		return json_decode( $jsonCode, true /* as array */ );
	}
	
	public function askApi4ri()
	{
		$this->setWebUrl( 'https://dora.lib4ri.ch/help.api/?issn=' . $this->searchTerm . ' &wanted=' . implode(',',$this->detailAry) );	// tmp/aux!
		$this->setApiUrl( rtrim( W_S_API_LIB4RI_URL, '/' ) . '/?issn=' . $this->searchTerm . ' &wanted=' . implode(',',$this->detailAry)	);
		// for example: http://152.88.205.16/web/api/?issn=1475-939X,1432-5233&wanted=agreement,jif

		$apiPrio = 7;
 
		// TO BE TUNED - EXPORT THIS VARIABLE CSS!
		$cssTrend = 'display:inline-block; margin-left:3px; font-size:1.25em; line-height:.8em; color:#777; width:1em; height:1em; transform: rotate([@deg]deg); -ms-transform:rotate([@deg]deg);';

		if ( in_array('jif',$this->detailAry) ) {

			$issnList = implode(',',$this->issnAry);
			if ( strpos($issnList,'-') ) { // $issnList to be e.g.: 1475-939X,1432-5233
				$resultAry = [];
				$infoAmount = 0;
				$markup = '';
				// JIFs are legally restricted! Show it only with the Lib4RI network!
				if ( websearch_ip_from('nerd') ) {
					$infoAmount = 2;
					$resultAry = $this->askApi4riHelper($issnList,'jif,oa-gold');
				} elseif ( websearch_ip_from('eawag') || websearch_ip_from('empa') || websearch_ip_from('psi') || websearch_ip_from('wsl') ) {
					$infoAmount = 1;
					$resultAry = $this->askApi4riHelper($issnList,'jif');
				}
				if ( $infoAmount && sizeof($resultAry) ) {
					$jifAry = [];
					$goldAry = [];
					foreach( $resultAry as $issn => $dataAry ) {
						if ( @!empty($dataAry['jif']) ) {
							foreach( $dataAry['jif'] as $year => $jif ) {
								if ( @isset($jifAry[$year]) ) { continue; }
								$jifAry[$year] = array( 'jif' => $jif, 'issn' => $issn, 'year' => $year );
							}
						}
						if ( $infoAmount > 1 && @!empty($dataAry['oa-gold']) ) {
							foreach( $dataAry['oa-gold'] as $year => $oaGold ) {
								if ( @isset($goldAry[$year]) ) { continue; }
								$goldAry[$year] = array( 'oa-gold' => $oaGold, 'issn' => $issn, 'year' => $year );
							}
						}
					}
					if ( !empty($jifAry) ) { // Display: Journal Impact Factor
						krsort($jifAry);	// so newest is first in array
						$ary = array_values($jifAry);		// like this we do not need to know the year
						if ( ( $jif = strval($ary[0]['jif']) /* floating number or 'n/a' */ ) && @strpos('/0123456789/',substr($jif,0,1)) ) {
							$markup = '<span title="JIF from ' . $ary[0]['year'] . '">' . strval($jif) . '</span>';
					//		$markup = 'http://gateway.webofknowledge.com/gateway/Gateway.cgi?GWVersion=2&SrcAuth=ExLibris&SrcApp=Lib4RI_primo&KeyISSN=' . $ary[0]['issn'] . '&DestApp=IC2JCR';
					//		$markup = '<a href="' . $markup . '" target="_blank" title="JIF from ' . $ary[0]['year'] . " \r\n" . 'Check the full record at Clarivate">' . strval($jif) . '</a>';
							if ( $infoAmount > 1 && ( $jifPrev = @floatval($ary[1]['jif']) ) && ( intval($ary[0]['year']) - @intval($ary[1]['year']) === 1 ) ) {
								$jifNow = floatval($jif);
								$deg = max($jifNow,$jifPrev) / min($jifNow,$jifPrev) - 1.0;     // tmp!
								$ratio = atan( $deg * 1.5 );	// 'over-scaling' a bit 
								$color = '#ff0';
								if ( $ratio > 0.0 ) {
									$dec = 255 - ceil( min($ratio,1.0) * 255.0 );
									$color = '#' . substr( '00' . dechex( $dec ), -2 ) . 'ff00';
								} elseif ( $ratio < 0.0 ) {
									$dec = 255 + floor( max($ratio,-1.0) * 255.0 );
									$color = '#ff' . substr( '00' . dechex( $dec ), -2 ) . '00';
								}
								$deg = atan( $deg * 2.0 ) * 180.0 / pi(); // proprietary scaling: with 2.0 we get 45deg for a jif grown by 50%
								if ( $jifPrev < $jifNow ) { $deg = 0.0 - $deg; }
								$tmp = 'JIF ' . $ary[1]['year'] . ': ' . $ary[1]['jif'] . "\r\n" . 'JIF ' . $ary[0]['year'] . ': ' . $jif;
								$markup .= '<div style="' . str_replace('[@deg]',$deg,$cssTrend) . '; color:' . $color . '; text-shadow: 1px -1px 2px #333, -1px 1px 1px #333, -1px -1px 1px #111, 1px 0px 2px #333, 1px 2px 2px #222, 2px 2px 4px #111;" title="' . $tmp . '">&#10142;</div>';
							}
							$tmp = 'http://gateway.webofknowledge.com/gateway/Gateway.cgi?GWVersion=2&SrcAuth=ExLibris&SrcApp=Lib4RI_primo&KeyISSN=' . $ary[0]['issn'] . '&DestApp=IC2JCR';
							$markup .= ' (<a href="' . $tmp . '" target="_blank" title="Check the full record at Clarivate">details</a>)';
						}
					}
					if ( $infoAmount > 1 && sizeof($goldAry) ) { // Display: % Gold Open Acess
						krsort($goldAry);	// so newest is first in array
						$ary = array_values($goldAry);		// like this we do not need to know the year
						if ( $oaGold = @trim($ary[0]['oa-gold']) ) {
							$oagHtml = '<span title="' . $oaGold . ' Gold Open Access articles in ' . $ary[0]['year'] . ' ">' . $oaGold . '</span>';
							if ( ( $oagPrev = floatval($ary[1]['oa-gold']) ) && ( intval($ary[0]['year']) - intval($ary[1]['year']) === 1 ) ) {
								$oagNow = floatval($oaGold);
								$deg = max($oagNow,$oagPrev) / min($oagNow,$oagPrev) - 1.0;     // tmp!
								$deg = atan( $deg * 2.0 ) * 180.0 / pi(); // proprietary scaling: with 2.0 we get 45deg for a jif grown by 50%
								if ( $oagPrev < $oagNow ) { $deg = 0.0 - $deg; }
								$tmp = '% OA Gold ' . $ary[1]['year'] . ': ' . $ary[1]['oa-gold'] . "\r\n" . '% OA Gold ' . $ary[0]['year'] . ': ' . $oaGold;
								$oagHtml .= '<div style="' . str_replace('[@deg]',$deg,$cssTrend) . '" title="' . $tmp . '">&#10142;</div>';
							}
							$markup .= '<div class="lib4ri-journal-area-col-data" style="padding-left:revert;">&#37; of Gold Open Access articles: ' . str_replace('%','&#37;',$oagHtml) . '</div>';
						}
					}
				}
				if ( empty($markup) ) {
					$issn = reset( array_slice( $this->issnAry, -1 ) );
					$markup = 'http://gateway.webofknowledge.com/gateway/Gateway.cgi?GWVersion=2&SrcAuth=ExLibris&SrcApp=Lib4RI_primo&KeyISSN=' . $issn . '&DestApp=IC2JCR';
					$markup = '<a href="' . $markup . '" target="_blank" title="You possibly may find the JIF at Clarivate">check online</a>';
				}
				$this->updateReturnHtml('jif','Journal Impact Factor: '.$markup /* , $apiPrio */ );
			}
		}

		if ( in_array('agreement',$this->detailAry) ) {
			$markup = '';
			$issnList = implode(',',$this->issnAry);
			if ( strpos($issnList,'-') ) {          // should be something like: 1475-939X,1432-5233
				$resultAry = $this->askApi4riHelper($issnList,'agreement');
				foreach( $resultAry as $issn => $dataAry ) {
					if ( @!empty($dataAry['agreement']) ) {
						foreach( $dataAry['agreement'] as $agreeType => $dataAry ) {
					//		if ( @!empty($dataAry) ) {
					//			$markup = $agreeType . ( ( substr($agreeType,-8) == 'greement' ) ? '' : ' Agreement' );
					// 2022-06-23: Use different term than in table:
							if ( @!empty($dataAry) && stripos($agreeType,'Open Access') !== false ) {
								$markup = 'Lib4RI Open Access Agreement';
								if ( $tmp = @trim($dataAry['Link']) ) {
							//		$markup = '<a href="' . $tmp . '" target="_blank">' . $markup . '</a>';
									$markup .= ' (<a href="' . $tmp . '" target="_blank">details</a>)';
									$markup .= ' <ul class="lib4ri-ul-flat">' . '<li class="lib4ri-li-wide">available</li>' . '</ul>';
								}
								if ( $tmp = @trim($dataAry['Licensing']) ) {
									$markup .= ' <ul class="lib4ri-ul-flat">' . '<li class="lib4ri-li-wide">License: ' . $tmp . '</li>' . '</ul>';
								}
								break;
							}
						}
					}
				}
			}
			if ( !empty($markup) ) {
				$this->updateReturnHtml('agreement', $markup, $apiPrio);
		//		$this->updateReturnHtml('embargo-fund', '&nbsp;', $apiPrio); // just to block this field/area, no Sherpa/DOAJ information wanted there!
			}
		}
		
	}
	
	public function askSwisscovery()
	{
		// TEST:
		// http://lib-cms-dev1/web/search.handler.php?req=json&api=JournalDetail&remote=swisscovery&issn=2673-4141&alma=99116818663005522&detail=available,linklist&target=lib4ri-journal-@-0&find=hydrogen
		
		if ( !in_array('available',$this->detailAry) && !in_array('linklist',$this->detailAry)  ) { return; }

		// Debug info:
		// Currently keeing full working links here for easier debugging, then replacing some parameters:
		$repAry = array(	/* note: API is using 'multiFacets=' while web link has 'mFacet=' */
			'Entomology'  => rawurlencode( $this->cleanTerm($this->searchTermOrig) ),
			'&limit=10&'  => '&limit=' . $this->searchLimit . '&',	/* seems to support only steps of 10, 25, 50 !? */
			'&offset=0&'  => '&offset=' . $this->searchOffset . '&',	/* not tuned so far */
		);

	//	$apiPrio = 0;	// lostest priority (by logic, resp. not used here explicitly).
	//	Reminded, this is intended to be basic/initial request and we forward requested data pieces to lib4ri-bentobox.js as we got them from SLSP.

		if ( empty($this->almaId) ) {
			$this->setWebUrl( str_replace(		/* to be replaced, just fully working links added here for debug reasons */
				array_keys($repAry),
				array_values($repAry),
				'https://lib4ri.swisscovery.slsp.ch/discovery/search?query=title,contains,Entomology&pfilter=rtype,exact,journals&tab=LibraryCatalog&search_scope=MyInstitution&vid=41SLSP_LIB:GO&mfacet=tlevel,include,online_resources,1&limit=10&offset=0&',
			) );
		} else {	/* search in title, only online journals */
			$this->setWebUrl( str_replace(		/* to be replaced, just fully working links added here for debug reasons */
				array('alma99116818663005522'),
				array($this->almaId),
				'https://lib4ri.swisscovery.slsp.ch/discovery/fulldisplay?docid=alma99116818663005522&context=L&vid=41SLSP_LIB:GO',
			) );
		}

		if ( empty($this->almaId) ) {
			$this->setApiUrl( str_replace(		/* to be replaced, just fully working links added here for debug reasons */
				array_keys($repAry),
				array_values($repAry),
				'https://lib4ri.swisscovery.slsp.ch/primaws/rest/pub/pnxs?blendFacetsSeparately=false&disableCache=false&getMore=0&inst=41SLSP_LIB&lang=en&limit=10&offset=0&multiFacets=facet_tlevel,include,online_resources&newspapersActive=false&newspapersSearch=false&offset=0&pcAvailability=true&q=title,contains,Entomology,AND;rtype,exact,journals&qExclude=&qInclude=&rapido=false&refEntryActive=true&rtaLinks=true&scope=MyInstitution&searchInFulltextUserSelection=true&skipDelivery=Y&sort=rank&tab=LibraryCatalog&vid=41SLSP_LIB:GO&'
			) );
		} else {
			$this->setApiUrl( str_replace(		/* to be replaced, just fully working links added here for debug reasons */
				array('alma99116818663005522'),
				array($this->almaId),
			/*	'https://lib4ri.swisscovery.slsp.ch/primaws/rest/pub/pnxs/L/alma99116818663005522?vid=41SLSP_LIB:GO&lang=en'	*/
				'https://lib4ri.swisscovery.slsp.ch/primaws/rest/pub/edelivery/alma99116818663005522?vid=41SLSP_LIB:GO&lang=en&googleScholar=false&'
		//		Test:
		//		https://lib4ri.swisscovery.slsp.ch/primaws/rest/pub/edelivery/alma99116727441905522?vid=41SLSP_LIB:GO&lang=en&googleScholar=false&
			) );
		}
		
		$jsonCode = $this->makeQuery( $this->apiUrl, 60 );		// large timeout recommanded
		$jsonObj = $this->jsonDecode( $jsonCode, false );

		$numFound = -1;
		if ( @empty($this->almaId ) ) {
			$numFound = ( @isset($jsonObj->info->total) ? @intval($jsonObj->info->total) : -1 );
		} else {	// amount of availalbe online versions:
			$numFound = sizeof($jsonObj->electronicServices); // should be the same as sizeof($jsonObj->GetIt1[0]->links)
		}
		
		foreach( $this->detailAry as $wanted /* so far only available + linklist */ ) {

			$markup = '';
			if ( $wanted == 'linklist' ) {	/* convention: everything wiht 'list' inside will be extended, not replaced */
				$markup = 'Further Information:&nbsp; <a href="' . $this->webUrl . '" target="_blank">' . 'swisscovery' . '</a>';
				foreach( $jsonObj->link as $linkData ) {		// test: forward link
					if ( strpos($linkData->linkURL,'DestApp=IC2JCR') ) { /* would link to the WoS JIF we are going to link ourself */ continue; }
					$tmp = @trim(str_replace('&nbsp;',' ',strip_tags($linkData->displayLabel)));
					$tmp = strtok( preg_replace('/\s+/',' ',$tmp) . ' - ' , ' - ' );	// cut at ' - '
					$markup .= ' | <a href="' . $linkData->linkURL . '" target="_blank" title="' . $tmp . ' Link by SLSP">' . $tmp . '</a>';
				}
			}

			elseif ( $wanted == 'available' ) {
				$markup = @trim(strip_tags($jsonObj->consolidatedCoverage),".; \n\r\t\v\x00");
				$linkAry = array(
					'own' => '' /* absolute/infinite link to SLSP resolver */ ,
					'ext' => '' /* external (ALMA) link with https://... */ ,
				);
				foreach( $jsonObj->electronicServices as $journal ) {
					if ( empty($markup) && !( $markup = @trim(strip_tags($journal->availability),".; \n\r\t\v\x00") ) ) {
						$markup = @trim(strip_tags($journal->availiability),".; \n\r\t\v\x00");	// there is a TYPO!?
					}
					if ( $url = @trim(strip_tags($journal->serviceUrl)) ) {
						if ( substr($url,0,4) == 'http' ) {
							if ( empty($linkAry['ext']) ) { $linkAry['ext'] = $url; }
						} else {
							if ( empty($linkAry['own']) ) { $linkAry['own'] = 'https://lib4ri.swisscovery.slsp.ch/' . ltrim($url,'/'); }
						}
					}
				}
				if ( stripos($markup,'Available') !== 0 ) {
					$markup = rtrim( 'Available online ' . ltrim($markup) );
				}
				if ( !empty($linkAry['own']) || !empty($linkAry['ext']) ) {
					$markup = '<a href="' . ( !empty($linkAry['own']) ? $linkAry['own'] : $linkAry['ext'] ) . '" target="_blank">' . $markup . '</a>';
				}
			}

			if ( !empty($markup) ) {
				if ( @empty($this->htmlTarget) ) {
					$this->returnAry['html'] = @strval($this->returnAry['html']) . ( @empty($this->returnAry['html']) ? '' : '<br>' ) . $markup;
				} else {
					$htmlId = str_replace('@',$wanted,$this->htmlTarget);		// id of HTML element
					$tmpAry = array( 'html' => array( $htmlId => $markup ) );
					$this->returnAry = ( @!is_array($this->returnAry) ? $tmpAry : array_merge_recursive( $this->returnAry, $tmpAry ) );
				}
			}
		}
	}

	public function queryNow($searchTerm = '', $returnMode = 0 )
	{
		if ( $searchTerm = $this->cleanTerm($searchTerm) ) {
			$this->searchTerm = $searchTerm;
		}

		if ( empty($this->almaId) && @!sizeof($this->issnAry) ) {
			$this->exitByError( 'ERROR: got no ID(s) !?' );
		}
		if ( @!is_array($this->detailAry) || !sizeof($this->detailAry) ) {
			$this->exitByError( 'ERROR: Don\'t know which details' );
		}
		if ( @empty($this->htmlTarget) ) {
			$this->exitByError( 'ERROR: Don\'t know where to place details' );
		}
		/*
		$apiClassName = 'apiQueryIssn' . ucFirst(strtolower($this->apiRemote));
		if ( !method_exists($apiClassName,'__construct') ) {
			$this->exitByError( 'ERROR: could not find API class ' . $apiClassName . '()' );
		}
		*/
		$callMe = 'ask' . ucFirst(strtolower($this->apiRemote));
		if ( !method_exists('apiQueryJournalDetail',$callMe) ) {
			$this->exitByError( 'ERROR: could not call ' . $callMe . '()' );
		} else {
			$this->$callMe();
		}

		return $this->jsonOutput( $this->returnAry, $returnMode );
	}
}



// -----------------------------------------------------------------------------------


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
	
/*
// ///////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////
*/
