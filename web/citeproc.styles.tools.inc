<?php
/*
	function to generate a list of the locally installed CSL styles.
	By default the function will try to cache the styles.
	
	Note the the apostrophe will be escaped with a backslash.
*/
function websearch_citeproc_style_list( $varName = 'stylesOnDisc', 
	$cacheFile = './citeproc.styles.list-local.inc', 
	$cslDir = './citeproc/vendor/citation-style-language/styles-distribution' ) {

	if ( empty($varName) || empty($cacheFile) || @filesize($cacheFile) < 50 ) {
		$styleAry = array();
		$cslDir = rtrim($cslDir,"/ \n\r\t\v\x00") . '/';		// ensure ending '/' !
		$scanAry = ( empty($cslDir) ? array() : @scandir($cslDir) );
		foreach( $scanAry as $cslFile ) {
			if ( strtolower(substr($cslFile,-4)) != '.csl' ) { continue; }
			if ( !( $xml = @simplexml_load_string(file_get_contents($cslDir.$cslFile)) ) ) { continue; }
			$tF = @trim( $xml->info->title );
			$styleAry[strtolower($tF)]['file'] = substr($cslFile,0,-4);
			$styleAry[strtolower($tF)]['full'] = $tF;
			$tS = 'title-short';
			if ( $tS = @trim( $xml->info->$tS ) ) {
				$styleAry[strtolower($tF)]['short'] = $tS;
			}
		}
		if ( !empty($varName) && !empty($cacheFile) ) {
			@file_put_contents($cacheFile,'<'.'?'."php\r\n\$".$varName.' = '.var_export($styleAry,true).';');
		}
		return $styleAry;
	}
	@include_once($cacheFile);
	return ( @isset($$varName) ? $$varName : NULL );
}

/*
	helper function to only return the CSL style that have an abbreviation
*/
function websearch_citeproc_style_list_short() {
	$shortList = array();
	$fullList = websearch_citeproc_style_list();
	foreach( $fullList as $cIdx => $cslAry ) {
		if ( @!empty($cslAry['short']) ) { $shortList[$cIdx] = $cslAry; }
	}
	return $shortList;
}

// preset selection of CSL citation files:
$cslAry = array(
	'american-medical-association',
	'national-institute-of-health-research',
	'national-library-of-medicine-grant-proposals',
	'springer-vancouver-author-date',
	'springer-vancouver-brackets',
);


