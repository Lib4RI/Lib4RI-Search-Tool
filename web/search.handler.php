<?php
/*
	search.handler.php	?req	=	json
					&find	=	<what you want>
					&api	=	<one of the known APIs e.g. scopus, dora, slsp, ...>
					&scope	=	<optional, to hold the institute for DORA, or the catalog for SLSP>
					&limit	=	<about of results to treat>
					&style	=	<filename of CiteProc CSL file>
					&lang	=	<language, e.g. en-US>
*/


// //////////// vvvvv /////////// MISC. CONFIG //////////// vvvvv /////////////

set_time_limit( 60 );	// bascially for SLSP/swisscovery (only)

@include_once('./search.config.inc');

$_dirAry = array( 
	'local' => '/var/www/html/web/',
	'online' => '/web/',
);

$_mimeAry = array(
	'json' => 'application/json',	/* only this is currently/intentionally supported */
	'text' => 'plain/text',
	'html' => 'application/html',
);
if ( !( $_reqFormat = @trim(strip_tags($_GET['req'])) ) || !in_array($_reqFormat,$_mimeAry) ) {
	$tmpAry = array_keys($_mimeAry);
	$_reqFormat = $tmpAry[0];
}

$cssAuxAry = array(	/* related to the Scopus-1-Author issue, to be exported */
	'.csl-left-margin {display:none; }',
	'.lib4ri-bentobox-searchterm { font-weight:700; font-style:italic; }',
);


// //////////// vvvvv ///////// FUNCTIONS + CLASSES /////// vvvvv /////////////

include_once($_dirAry['local'].'search.handler.functions.inc');	// providing 'exit functions'
include_once($_dirAry['local'].'search.handler.classes.inc');	// will need global $_reqFormat + $_mimeAry


// //////////// vvvvv ////////////// IP CHECKS //////////// vvvvv /////////////

$_websearch_access_check_bot = true;
$_websearch_access_check_ip = false;
if ( !( @include_once($_dirAry['local'].'search.protect.inc') ) ) {		// trivial access check - include it first!
	_lib4ri_exitByError('401');
}
if ( websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named('black'), false, true ) ) {
	_lib4ri_exitByError('403');
}
if ( !websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named('white'), true ) ) {
	_lib4ri_exitByError('403');
}
$_is_scopus_or_wos = ( ( $tmp = @strtolower(trim(strip_tags($_GET['api']))) ) && in_array($tmp,array('scopus','wos')) );
$_is_intranet = websearch_ip_from('lib4ri');



// //////////// vvvvv //////// API + STYLING SUPPORT ////// vvvvv /////////////

include_once($_dirAry['local'].'citeproc.styles.tools.inc');	// for websearch_citeproc_style_list_short()
include_once($_dirAry['local'].'meta-remap.tools.inc');
include_once($_dirAry['local'].'search.classes.inc');
include_once($_dirAry['local'].'search.classes.journal.inc');



// //////////// vvvvv //// PROCESSING FUN STARTS HERE: //// vvvvv /////////////

$getJsonAPI = null;

// Special case (legacy support): Produce HTML link tags as we had on the old lib4ri.ch search page.
// Quite proprietary handling... - partially could be done with JS too, so this probably needs to be revised/removed.
if ( @!empty($_GET['linkset']) ) {
	$getJsonAPI = new searchHandler();
	$getJsonAPI->getJsonLinkSet( trim(strip_tags($_GET['linkset'])) );
	exit;
}

if ( !( $apiName = @strtolower(trim(strip_tags($_GET['api']))) ) ) {
	_lib4ri_exitByError( "ERROR: API not URL-assigned." );
}


// //////////// vvvvv ///// API class/object selection //// vvvvv /////////////

$scopeTmp = trim( @strip_tags($_GET['scope']) );
switch ( $apiName ) {
	case 'google':
		$searchTerm = @trim(strip_tags(rawurldecode($_GET['find'])));		// OK/standable if empty
		if ( $scopeTmp != 'maps' ) {
			header('Location: https://www.google.com/search?q=' . rawurlencode($searchTerm) );
		}
		elseif ( $twigTemplate = @file_get_contents('./twig/lib4ri-google-maps.twig') ) {
			header('Content-Type: ' . 'text/html' . ' charset=utf-8');
			print( _lib4ri_twigSimple( array( 'locHtml' => htmlentities($searchTerm), 'locUrl' => rawurlencode($searchTerm) ) , $twigTemplate ) );
		} else {
			header('Location: https://maps.google.com/maps?q=' . rawurlencode($searchTerm) );
		}
		exit;
	case 'scopus':
		$_GET['scope'] = null;
		$getJsonAPI = new apiQueryScopus();
		break;
	case 'slsp':	/* legacy support */
		$getJsonAPI = new apiQuerySLSP( $scopeTmp );
		break;
	case 'swisscovery':	/* new name */
		$getJsonAPI = new apiQuerySLSP( $scopeTmp );
		break;
	case 'journal':
		$getJsonAPI = new apiQueryJournal( $scopeTmp );
		break;
	case 'dora':
		$getJsonAPI = new apiQueryDORA( $scopeTmp );
		break;
	default: /* try to find the class/object automatically depending on the API's name: */
		$apiObjName = 'apiQuery' . ucfirst(trim(strip_tags($_GET['api'])));
		if ( @!class_exists($apiObjName,false) ) {
			_lib4ri_exitByError( "ERROR: API class '" . $apiObjName . "' not found." );
		}
		try {
			if ( empty($scopeTmp) ) {
				$getJsonAPI = new $apiObjName();
			} else {
				$getJsonAPI = new $apiObjName( $scopeTmp );
			}
		}
		catch (Error $e) {
			_lib4ri_exitByError( "ERROR: Calling API class '" . $apiObjName . "' failed. " . $e->getMessage() );
		}
}


if ( !( $searchLanguage = @trim(strip_tags($_GET['lang'])) ) ) { // for CiteProc, case sensitive!
	$searchLanguage = 'en-US';
}
$searchLanguage = strtolower(substr($searchLanguage,0,2)) . @strtoupper(substr($searchLanguage,2)); // Check! CiteProc wants/needs it this way!
$getJsonAPI->apiLanguage = $searchLanguage;

$apiName = $getJsonAPI->apiName;
$apiHost = ( @empty($getJsonAPI->apiHost) ? $apiName : strtolower($getJsonAPI->apiHost) );
$urlRemoteSearch = $getJsonAPI->webUrl($getJsonAPI->searchTermOrig);

// API link for tests: $urlRemoteSearch = $getJsonAPI->apiUrl($getJsonAPI->searchTermOrig, ( $getJsonAPI->searchLimit + $getJsonAPI->searchOffset ) );
// Test output:
if ( @intval($_GET['dev-test']) == 1 && websearch_ip_from('dev') ) { echo print_r( $getJsonAPI->apiUrl( @empty($_SERVER['HTTP_HOST']) ? 'hydro' : '' ), 1 ); return; }


// If JS could not find the/a search form then forward the hard-coded one:
if ( @!empty($_SERVER['HTTP_HOST']) && method_exists($getJsonAPI,'showForm') && strlen(trim(urldecode($getJsonAPI->searchTermOrig))) < 2 ) {
	$getJsonAPI->showForm();
}

$jsonCode = '[]';
$jsonAry = array();
if ( $_is_intranet || !$_is_scopus_or_wos ) {
	$jsonCode = ( @empty($_SERVER['HTTP_HOST']) ? $getJsonAPI->queryNow('hydro'/* console test */) : $getJsonAPI->queryNow(/* for 'find=' from URL */) );
	$jsonAry = json_decode($jsonCode, true /* TRUE for an array, note that some field names from the APIs may have 'illegal' charaters */ );
}

if ( @intval($_GET['dev-test']) == 2 && websearch_ip_from('dev') ) { echo print_r( json_encode($jsonAry,JSON_PRETTY_PRINT), 1 ); exit; }

if ( empty($jsonCode) ) {
	_lib4ri_exitByError( "ERROR: got no JSON code" );
}
if ( @strval($jsonAry->query_status) == 'error' ) {
	_lib4ri_exitByError( "ERROR: API query failed (see debug option 3 or 4)" );
}


// Scopus-1-Author issue: Sometimes it's safer to treat chech result individuelly by CiteProc.
// The 'benefit' is we only may loose a single result, not all (since rendered as a group, with enumeration).
// However we must afterwards put all single citeproc'ed resutls together again as Citeproc would,
// we also need to hide CSS classe csl-left-margin because the related <div> tag shows 1.) always.
$cite1by1 = @boolval( $metaMap['citeproc'][$apiHost]['_cite_1_by_1'] ); // see citeproc.styles.tools.inc

$json4cp = ( ( $cite1by1 || $apiName == 'scopus' ) ? array() : '' );

if ( @!empty($jsonAry) ) {
	$json4cp = websearch_json_remap( 
		$jsonAry,
		$apiHost,
		'citeproc',
		( ( $cite1by1 || $apiName == 'scopus' ) ? -1 : 1 )	/* -1: array, 1: code/text */
	);
}

// Test output:
if ( @intval($_GET['dev-test']) == 6 && websearch_ip_from('dev') ) { echo print_r( $json4cp, true ); exit; }


// Special case / Scopus-1-Author issue:
if ( $apiName == 'scopus' && is_array($json4cp) && sizeof($json4cp) ) {
	// Scopus will only show 1 author, let check Crossref (and Scopus' Abstact API) for more.
	// If there are, we are going to expand authors for each result in $json4cp array:
	include_once($_dirAry['local'].'search.handler.author-aux.inc');
}


// Test output:
if ( @intval($_GET['dev-test']) == 7 && websearch_ip_from('dev') ) { echo print_r( $json4cp, true ); exit; }


$numFound = 0;
if ( $apiName == 'wikipedia' ) {	// work-around, to be tested/revised
	$numFound = $getJsonAPI->getNumFound();
} elseif ( $_is_intranet || !$_is_scopus_or_wos ) {
	$numFound = @intval( websearch_json_value_by_path( $jsonAry, $metaMap['citeproc'][$apiHost]['_numFound'] ) );
}

$searchLimit = $getJsonAPI->searchLimit;
$no_item_listing = boolval( stripos('/national/',$getJsonAPI->apiScope) );



// //////////// vvvvv ////// RENDERING HTML RESPONSE ////// vvvvv /////////////

// Compose preset html areas to show inside the API-related bentobox sub-area.
// However all required data will additionally send piece by piece for more post-production options.
$htmlAry = array( 'header' => '', 'center' => '', 'footer' => '' );

// ------------------------------------------------------------------------
// Bentobox header:
$htmlAry['header'] = 'Found <a href="' . $urlRemoteSearch . '" target="_blank">' . $numFound . ' result' . ( $numFound === 1 ? '' : 's' ) . '</a>';
$htmlAry['header'] .= ' with <span class="lib4ri-bentobox-searchterm">' . trim(strip_tags($getJsonAPI->searchTermOrig)) . '</span>';
$htmlAry['header'] .= ' at ' . ltrim(strrchr('/'.$getJsonAPI->apiLabel,'/'),'/ ');
if ( $searchLimit > 0 ) {
	if ( $numFound > $searchLimit ) {
		$htmlAry['header'] .= ' - the top ' . strval($searchLimit) . ' result' . ( $searchLimit === 1 ? '' : 's' );
	}
	$htmlAry['header'] .= ':';
}

// ------------------------------------------------------------------------
// Bentobox footer:
$htmlTmp = ltrim(strrchr('/'.$getJsonAPI->apiLabel,'/'),'/ ');
$isDev = websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named('dev'), true );
if ( $numFound == 1 ) {
	if ( $htmlTmp != 'Journal List' || $isDev ) {
		$htmlAry['footer'] .= 'See this <a href="' . $urlRemoteSearch . '" target="_blank">result on';
		$htmlAry['footer'] .= ( $htmlTmp == 'Journal List' ? /* ' swisscovery Lib4RI' */ 'line' : ' '.$htmlTmp ) . '</a>';
		if ( $isDev ) { $htmlAry['footer'] = '{ Dev: ' . $htmlAry['footer'] . ' }'; }
	} else {
		$htmlAry['footer'] = '<!-- &nbsp; -->';
	}
} elseif ( $numFound > 1 ) {
	if ( $htmlTmp != 'Journal List' ) {
		$htmlAry['footer'] .= 'See <a href="' . $urlRemoteSearch . '" target="_blank">all ' . $numFound . ' results</a>';
	}
	elseif ( $numFound > $searchLimit || $isDev ) {
		$sOff = $getJsonAPI->searchOffset;
		$sLim = $getJsonAPI->searchLimit;
		if ( $isDev ) {
			$htmlAry['footer'] .= '{ Dev: ' . 'Found overall <a href="' . $urlRemoteSearch . '" target="_blank">' . $numFound . ' result' . ( $numFound == 1 ? '' : 's' ) . '</a>' . ' } ';
		}
		if ( $sLeft = max($numFound-$sOff-$sLim,0) ) {
			$htmlTmp = 'result' . ( $sLeft > 1 ? 's '.($sLim+$sOff+1).'-'.($sLim+$sOff+min($sLeft,$sLim)) : ' '.($sLim+$sOff+1) );	// result 'bandwidth', e.g. "results 41-60"
			$htmlAry['footer'] .= '<span id="' . 'lib4ri-bentobox-next-desc-' . ($sLim+$sOff) . '">See ';
			$htmlAry['footer'] .= '<a id="' . 'lib4ri-bentobox-next-link-' . ($sLim+$sOff) . '" href="javascript:" ';
			$htmlAry['footer'] .= 'onclick="javascript:lib4riSearchBentoboxNext(\''.$getJsonAPI->searchTerm.'\','.($sOff+$sLim).',this.id,\'below '.$htmlTmp.'\');"';
			$htmlAry['footer'] .= '>' . $htmlTmp . '</a></span>';
		}

	} else {
		$htmlAry['footer'] = '<!-- &nbsp; -->';
	}
} elseif ( !$_is_intranet && $_is_scopus_or_wos ) { // no results because restricted because outside IP range:
	$htmlAry['footer'] .= 'You are beyond the <span style="white-space:nowrap">Eawag/Empa/PSI/WSL</span> network. Result list is disabled.<br>';
	$htmlAry['footer'] .= 'If you have a login <a href="' . $urlRemoteSearch . '" target="_blank">check ' . $htmlTmp . '</a>.';
} else {
	$htmlAry['footer'] .= '<a href="' . $urlRemoteSearch . '" target="_blank">No results found</a>.';
}


// Special case:
if ( $apiHost == 'slsp' && $getJsonAPI->apiName == 'book' && $getJsonAPI->apiScope == "myinstitution" ) {
	$apiObjAux = new apiQueryBook('national');
	$jsonAux = $apiObjAux->queryNow();
	if ( $numAux = intval( websearch_json_value_by_path( json_decode($jsonAux,true), $metaMap['citeproc'][$apiHost]['_numFound'] ) ) ) {
		$htmlAry['footer'] .= '<br><br>Extend search to Swiss libraries via&nbsp;' . $apiObjAux->apiLabel . ':';
		$htmlAry['footer'] .= '<br>See <a href="' . $apiObjAux->webUrl() . '" target="_blank">all ' . $numAux . ' results</a>';
	} else {
		$htmlAry['footer'] .= '<br><br><a href="' . $apiObjAux->webUrl() . '" target="_blank">No results found</a> ';
		$htmlAry['footer'] .= 'when extending search<br>to Swiss libraries via&nbsp;' . $apiObjAux->apiLabel . ( $isDev ? '...&#128533;' : '.' );
	}
}




// //////////// vvvvv //////// CITEPROC INTEGRATION /////// vvvvv /////////////

use Seboettg\CiteProc\StyleSheet;
use Seboettg\CiteProc\CiteProc;


if ( $no_item_listing || ( !$_is_intranet && $_is_scopus_or_wos ) ) {
	$numFound += 0;			// dummy command, do nothing right now (just to cover this case)
}
elseif ( $apiName == 'wikipedia' ) {	// to be tested
	$htmlAry['center'] .= '<div class="csl-bib-body">' . $getJsonAPI->makeHtml($jsonAry,175,'csl-entry') . '</div>';
}
elseif ( $getJsonAPI->apiLabel == 'Journal List' ) {
	// Generate the tab content for the journal tab (trying to adopt Citeproc handling/look widely)
	include_once($_dirAry['local'].'search.handler.journal-tab.inc');
}
elseif ( $searchLimit > 0 && @include_once($_dirAry['local'].'citeproc/vendor/autoload.php') ) { // Bentobox Center with CiteProc Markup:

	$styleLoaded = null;
	try {
		$styleLoaded = StyleSheet::loadStyleSheet($getJsonAPI->citeStyle);
	}
	catch (Error $e) {
		$htmlAry['center'] .= '<i>We are sorry, the citation style \'' . $getJsonAPI->citeStyle . '\' could not be rendered (' . $e->getMessage() . ')</i>';
	}
	if ( $styleLoaded != null ) {
		if( !$cite1by1 ) {
			try {
				$citeProc = new CiteProc( $styleLoaded, $searchLanguage, websearch_citeproc_polish_markup($apiHost) );
				$htmlAry['center'] .= $citeProc->render( json_decode( is_string($json4cp) ? $json4cp : json_encode($json4cp) /* array to object */ ), "bibliography");
			}
			catch (Error $e) {
				$htmlAry['center'] .= '<i>We are sorry, we could not render the citations for the results (' . $e->getMessage() . ')</i>';
			}
		} elseif( is_array($json4cp) ) {
			$htmlAry['center'] .= '<div class="csl-bib-body">';	// we must add it once, remove it however from each result:
			foreach( $json4cp /* = array now */ as $jsonItem ) {
				try {
					$citeProc = new CiteProc( $styleLoaded, $searchLanguage, websearch_citeproc_polish_markup($apiHost) );
					$htmlTmp = $citeProc->render( json_decode(json_encode(array($jsonItem))), "bibliography");
					$htmlTmp = substr($htmlTmp,strpos($htmlTmp,'>')+1);
					$htmlAry['center'] .= substr($htmlTmp,0,strrpos($htmlTmp,'<'));
				}
				catch (Error $e) {
					$htmlAry['center'] .= '<div class="csl-entry"><i>We are sorry, we could not render the citations for the results (' . $e->getMessage() . ')</i></div>';
				}
			}
			$htmlAry['center'] .= '</div>';
		} else {
			$htmlAry['center'] .= '<i>We are sorry, there is technical matter when rendering the citations (unexpected data structure)</i>';
		}
	}
}



// //////////// vvvvv ///// FINAL RESPONSE TO ASYNC JS //// vvvvv /////////////

$htmlAry['header'] = @empty($htmlAry['header']) ? '' : '<div class="lib4ri-bentobox-header">' . $htmlAry['header'] . '</div>';
$htmlAry['center'] = @empty($htmlAry['center']) ? '' : '<div class="lib4ri-bentobox-center">' . $htmlAry['center'] . '</div>';
$htmlAry['footer'] = @empty($htmlAry['footer']) ? '' : '<div class="lib4ri-bentobox-footer">' . $htmlAry['footer'] . '</div>';

$htmlFull = '<div class="lib4ri-bentobox-' . $apiName . '" id="lib4ri-bentobox-' . $apiName . '">' . implode('',array_filter($htmlAry)) . '</div>';



if ( !empty($_reqFormat) && in_array($_reqFormat,array_keys($_mimeAry)) ) {
	header('Content-Type: ' . $_mimeAry[$_reqFormat] . ' charset=utf-8');
	print json_encode( array(
			'api-name' => $getJsonAPI->apiName,
			'api-label' => $getJsonAPI->apiLabel,
			'api-scope' => $getJsonAPI->apiScope,
			'total-found' => $numFound,
			'search-term' => rawurlencode( $getJsonAPI->searchTermOrig ),
			'search-limit' => $getJsonAPI->searchLimit,
			'url-remote' => $urlRemoteSearch,
			'html-css'   => @trim(implode("\r\n",$cssAuxAry)),
			'html' => rawurlencode( $htmlFull ),		/* safer, but we have decode this again */
	/*		'html-header' => $htmlAry['header'],
			'html-center' => $htmlAry['center'],
			'html-footer' => $htmlAry['footer'],		*/
		), JSON_PRETTY_PRINT );
	exit;
}

?>
