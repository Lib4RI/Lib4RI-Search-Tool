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
@include_once('./search.traits.inc');


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

$timeTest = 0.0;

// //////////// vvvvv ///////// FUNCTIONS + CLASSES /////// vvvvv /////////////

include_once($_dirAry['local'].'search.handler.functions.inc');	// providing 'exit functions'

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
include_once($_dirAry['local'].'search.handler.classes.inc');	// will need global $_reqFormat + $_mimeAry
include_once($_dirAry['local'].'search.classes.journal.inc');



// //////////// vvvvv //// PROCESSING FUN STARTS HERE: //// vvvvv /////////////

$getJsonAPI = null;
$timeTest = microtime(true);
$searchTerm = trim( @strip_tags( urldecode($_GET['find']) ) );		// also available then via classObject->getSearchTerm()

// Special case (legacy support): Produce HTML link tags as we had on the old lib4ri.ch search page.
// Quite proprietary handling... - partially could be done with JS too, so this probably needs to be revised/removed.
if ( $linkSet = @trim(strip_tags($_GET['linkset'])) ) {
	if ( stripos($linkSet,'Google') && stripos($linkSet,'Lib4RI') ) { // temp. exception: 'unter construction' for internal/lib4ri.ch search:
		$htmlAry = array(
			'<table style="border:0; margin:2ex 0 2ex 0; padding:0;"><tbody><tr><td style="margin-right:2ex;">',
			'<img src="https://svgsilh.com/png-512/150271-3f51b5.png" width="256" height="238" style="width:256px; height:238px;">',
			'<!-- image source: https://wordpress.org/openverse/image/b18cfb7f-58ee-4108-b935-7c3fb9d79ce5/ -->',
			'</td><td>&nbsp;</td><td style="vertical-align:top;">',
			'<br>With the relaunch of our website, you will see a a results list here. <br>Until then, please use this customized ',
			'<a href="https://www.google.com/search?hl=en&num=30&q=site%3Awww.lib4ri.ch%20' . rawurlencode($searchTerm) . '" target="_blank">Google link to search lib4ri.ch</a>',
			'.</td></tr></tbody></table>',
		);
		$retAry = array( 'set' => array( implode('',$htmlAry) ) );		// for compatibility, send 'set' as well!
		$retAry['html'] = '<div class="lib4ri-bentobox-linkset-link">' . $retAry['set'][0] . '</div>';
		$tmp = @trim( $GLOBALS['_reqFormat'] );
		header('Content-Type: ' . ( @empty($GLOBALS['_mimeAry'][$tmp]) ? 'text/plain' : $GLOBALS['_mimeAry'][$tmp] ) . ' charset=utf-8');
		print json_encode( $retAry, JSON_PRETTY_PRINT );
	}
	elseif ( $_SERVER['HTTP_HOST'] != '152.88.205.16' /* ALWAYS EXPECT OF DEV CURRENTLY */ || !stripos($linkSet,'Wikipedia') ) {
		$getJsonAPI = new searchHandler();
		$getJsonAPI->getJsonLinkSet($linkSet);
	}
	else {
		$getJsonAPI = new searchHandler();
		$jsonAry = $getJsonAPI->getJsonLinkSet($linkSet,'array');

		$getJsonAPI = new apiQueryWikihit();
		$tmpAry = json_decode($getJsonAPI->queryNow( strtr(ucWords($searchTerm),' ','_') ),true); // API is case-sensitive!(?) - better chances with UC words!(?)

	//	echo print_r( json_encode($tmpAry,JSON_PRETTY_PRINT), 1 ); exit;

		if ( @!empty($tmpAry['extract']) ) {

			//	$html = '<table border="0" cellpadding="0" cellspacing="0" style="margin-bottom:18px;"><tr><td>';
			//	$html .= '<img src="' . $tmpAry['thumbnail']['source'] . '" width="' . $tmpAry['thumbnail']['width'] . '" height="' . $tmpAry['thumbnail']['height'] . '" />';
			//	$html .= '</td><td style="vertical-align:top; padding:6px 0 0 12px;">';
			//	$html = '<a href="' . $tmpAry['content_urls']['desktop']['page'] . '" target="_blank">' . $html . '</a> ' . $tmpAry['extract_html'];
			//	$html .= '</td></tr></table><br>';

			$img = '<img src="' . $tmpAry['thumbnail']['source'] . '" height="' . /* min($tmpAry['thumbnail']['height'],200) . */ '180px" />';
			$img = '<a href="' . $tmpAry['content_urls']['desktop']['page'] . '" target="_blank">' . $img . '</a>';

			$ext = implode(' ',array_slice(explode(' ',$tmpAry['extract_html']),0,75));
			if ( $pos = strpos($ext,'</b>') ) {
				$ext = substr($ext,0,$pos+4) . '</a>' . substr($ext,$pos+4);
				if ( ( $pos = strpos($ext,'<b') ) !== false ) {
					$ext = substr($ext,0,$pos) . '<a href="' . $tmpAry['content_urls']['desktop']['page'] . '" target="_blank">' . substr($ext,$pos);
				}
			}
			$html = '<table border="0" cellpadding="0" cellspacing="0" style="margin:0px 0 ' . ( strlen($tmpAry['extract_html']) > strlen($ext) ? '15px' : '6px' ) . ' 0;""><tr>';
			$html .= '<td style="vertical-align:top; padding:3px 6px 6px 0;">' . $ext . ( strlen($tmpAry['extract_html']) > strlen($ext) ? '...' : '' ) . '</td>';
			$html .= '<td>' . $img . '</tr></table>';

			$pos = strpos($jsonAry['html'],'>') + 1;
			$jsonAry['html'] = substr($jsonAry['html'],0,$pos) . $html . substr($jsonAry['html'],$pos);
			$jsonAry['set'] = array_merge( array($html), $jsonAry['set'] );
		}

		$tmp = @trim( $GLOBALS['_reqFormat'] );
		header('Content-Type: ' . ( @empty($GLOBALS['_mimeAry'][$tmp]) ? 'text/plain' : $GLOBALS['_mimeAry'][$tmp] ) . ' charset=utf-8');
		print json_encode( $jsonAry, JSON_PRETTY_PRINT );
	}
	exit;
}

if ( !( $apiName = @strtolower(trim(strip_tags($_GET['api']))) ) ) {
	_lib4ri_exitByError( "ERROR: API not URL-assigned." );
}


// //////////// vvvvv ///// API class/object selection //// vvvvv /////////////

$scopeTmp = trim( @strip_tags($_GET['scope']) );

if ( @strval($_GET['api']) == 'resolve' && in_array($scopeTmp,['alma','dora']) ) {
	if ( ( $id = @trim($_GET[$scopeTmp]) ) || ( $id = @trim($_GET['id']) ) ) {
		$apiTest = new searchHandler();
		$apiTest->urlResolve($scopeTmp, $id ); // for alma: 99116717651105522
		// for example:
		// https://search.lib4ri.ch/web/search.handler.php?api=resolve&scope=alma&id=99116717651105522
		exit;
	}
}

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
	case 'dora':
		$getJsonAPI = new apiQueryDORA( $scopeTmp );
		break;
	case 'wikipedia':
		$getJsonAPI = new apiQueryWikihit();
		break;
	case 'wikihit':
		$getJsonAPI = new apiQueryWikihit();
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

// die( "Obj; " . $getJsonAPI->apiUrl() );

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

if ( @!empty($jsonAry) && @isset($metaMap['citeproc'][$apiHost]) && /* exceptions: */ !in_array($apiName,['wikihit']) ) {
	$json4cp = websearch_json_remap( 
		$jsonAry,
		$apiHost,
		'citeproc',
		( ( $cite1by1 || $apiName == 'scopus' ) ? -1 : 1 )	/* -1: array, 1: code/text */
	);
}

// Test output:
if ( @intval($_GET['dev-test']) == 5 && websearch_ip_from('dev') ) { echo "JSON for CiteProc:\r\n\r\n" . print_r( $json4cp, true ); exit; }


// Special case / Scopus-1-Author issue:
if ( $apiName == 'scopus' && is_array($json4cp) && sizeof($json4cp) ) {
	// Scopus will only show 1 author, let check Crossref (and Scopus' Abstact API) for more.
	// If there are, we are going to expand authors for each result in $json4cp array:
	include_once($_dirAry['local'].'search.handler.author-aux.inc');
}

// Test output:
if ( @intval($_GET['dev-test']) == 6 && websearch_ip_from('dev') ) { echo "JSON for CiteProc:\r\n\r\n" . print_r( $json4cp, true ); exit; }


$numFound = 0;
if ( 2 > 3 && substr($apiName,0,4) == 'wiki' ) {	// work-around, to be tested/revised, disabled currently
	$numFound = $getJsonAPI->getNumFound();
} elseif ( isset($metaMap['citeproc'][$apiHost]) && ( $_is_intranet || !$_is_scopus_or_wos ) ) {
	$numFound = @intval( websearch_json_value_by_path( $jsonAry, $metaMap['citeproc'][$apiHost]['_numFound'] ) );
}

$searchLimit = $getJsonAPI->searchLimit;
$no_item_listing = boolval( stripos('/national/',$getJsonAPI->apiScope) );


if ( @intval($_GET['dev-test']) == 8 && websearch_ip_from('dev') ) { echo "JSON Array:\r\n\r\n" . print_r( $jsonAry, true ); exit; }


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
		/*
			if ( $sLeft > $sLim ) {
				$htmlAry['footer'] .= ' | <span id="' . 'lib4ri-bentobox-next-desc-' . ($sLim+$sOff) . '"> see ';
				$htmlAry['footer'] .= '<a id="' . 'lib4ri-bentobox-next-link-' . ($sLim+$sOff) . '" href="javascript:" ';
				$htmlAry['footer'] .= 'onclick="javascript:lib4riSearchBentoboxNext(\''.$getJsonAPI->searchTerm.'\','.($sOff+999999).',this.id,\'below '.$htmlTmp.'\');"';
				$htmlAry['footer'] .= '>' . 'all results' . '</a></span>';
			}
		*/
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
	$apiObjNatio = new apiQueryBook('national');
	$jsonTmp = $apiObjNatio->queryNow();
	$numNatio = intval( websearch_json_value_by_path( json_decode($jsonTmp,true), $metaMap['citeproc'][$apiHost]['_numFound'] ) );
	$apiObjDisco = new apiQueryBook('discovery');
	$jsonTmp = $apiObjDisco->queryNow();
	$numDisco = intval( websearch_json_value_by_path( json_decode($jsonTmp,true), $metaMap['citeproc'][$apiHost]['_numFound'] ) );
	$htmlAry['footer'] .= '<br><br>Extend search: <ul class="lib4ri-ul-flat">';
	$htmlAry['footer'] .= '<li class="lib4ri-li-wide"><a href="' . $apiObjDisco->webUrl() . '" target="_blank">' . $numDisco . ' result' . ( $numDisco == 1 ? '' : 's' ) . '</a> in all ' . $getJsonAPI->apiLabel . ' libraries</li>';
	$htmlAry['footer'] .= '<li class="lib4ri-li-wide"><a href="' . $apiObjNatio->webUrl() . '" target="_blank">' . $numNatio . ' result' . ( $numNatio == 1 ? '' : 's' ) . '</a> including book chapters</li>';
	$htmlAry['footer'] .= '</ul>';
}



// //////////// vvvvv //////// CITEPROC INTEGRATION /////// vvvvv /////////////

use Seboettg\CiteProc\StyleSheet;
use Seboettg\CiteProc\CiteProc;


if ( $no_item_listing || ( !$_is_intranet && $_is_scopus_or_wos ) ) {
	$numFound += 0;			// dummy command, do nothing right now (just to cover this case)
}
elseif ( substr($apiName,0,4) == 'wiki' ) {
	$html = $getJsonAPI->makeHtml($searchTerm);
	$htmlAry['center'] .= '<div class="csl-bib-body" style="margin:0 0 ' . ( @empty($tmpAry['extract']) ? '.5ex' : '0' ) . ' 1em;">' . $html . '</div>';
	$htmlAry['footer'] = ''; // = '<div style="display:none;">' . $htmlAry['footer'] . '</div>';	// Hide (currently) not to see related results!(?)
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
	/*		'time-needed' => $getJsonAPI->timeNeeded,		*/
			'time-test' => ( microtime(true) - $timeTest ),
		), JSON_PRETTY_PRINT );
	exit;
}

?>
