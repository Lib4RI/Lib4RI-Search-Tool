<?php

trait apiQueryTraits
{
	/*
		function to decode an URL string.
		It will try to deocde as long as encoding are found, otherwise cut it off.
	*/
	public function cleanUrlInput( $term = '', $keepPercent = false )
	{
		$term = urldecode($term);	// let's try it once only!(?)
		$ary = [];
		while( preg_match('/\%[a-fA-F0-9]{2}/', $term, $ary, PREG_OFFSET_CAPTURE) ) {
			if ( !empty($ary[0][0]) /* what we found */ && @isset($ary[0][1]) /* where/postion */ ) {
				$term = rawurldecode( $term );
				if ( stripos($term,$ary[0][0]) === $ary[0][1] ) { // recheck to cut off
					$term = substr($term,0,intval($keepPercent)+$ary[0][1]) . substr($term,$ary[0][1]+3);
				}
			}
		}
		return $term;
	}

	public function stripTags( $term = '', $tagsOk = null, $preProcess = ['cleanUrlInput'])
	{
		// pre-process optionally (removing tags only make sense if it's fully decoded):
		$tuneAry = @array_slice($preProcess,0);
		foreach( $tuneAry as $funcName ) {
			if ( method_exists('apiQueryTraits',$funcName) ) {
				$term = apiQueryTraits::$funcName($term);
			}
			elseif ( function_exists($funcName) ) {
				$term = $funcName($term);
			}
		}

		$tagAry = [];
		if ( !empty($tagsOk) ) {
			$tagAry = ( is_array($tagsOk) ? $tagsOk : array_map('trim',explode(',',strval($tagsOk))) );
		}
		elseif ( @defined('W_S_HTML_TAGS_OK') ) {
			$tagAry = array_map('trim',explode(',',strval( W_S_HTML_TAGS_OK )));
		}

		if ( function_exists('filter_xss') ) { // by Drupal, probably safer!(?)
			$term = filter_xss( $term, $tagAry );
		} else {
			$term = strip_tags( $term, ( is_array($tagAry) ? '<'.implode('><',$tagAry).'>' : $tagAry ) ); // PHP 7.4 for array support
		}

		return trim( preg_replace('/\s+/',' ',$term) );
	}

}

trait apiQueryFutureTraits
{
	/*
		function to report an error message for/inside a json or text file
		Note, this is similar to method apiQueryUtils->exitByError in search.classes.inc - to be sync'ed/morged once!?
	*/
	public function exitByError( $eMsg = 'ERROR', $status = 'error', $asJson = true )
	{
		$eNum = intval($eMsg);
		if ( $eNum && @isset($GLOBALS['_websearch_error_list'][$eNum]) ) {	/* optional, by search.protect.inc */
			$eMsg = $eNum . ' ' . $GLOBALS['_websearch_error_list'][$eNum];
			header( 'HTTP/1.0 ' . ltrim($eMsg) );
		}
		header('Content-Type: ' . ( $asJson ? 'application/json' : 'text/plain' ). ' charset=utf-8');
		print ( $asJson ? json_encode( array( $status => trim($eMsg)), JSON_PRETTY_PRINT ) : trim($eMsg) );
		exit;	
	}
	

	/*
		Trivial/limited function to replace 'placeholder terms' in a TWIG template. For full support see https://twig.symfony.com
		An assoc input array is required with, e.g. array( 'term1inTwig' => 'value1forTwig', 'term2inTwig' => 'value2forTwig', ... )
		Conventions:
		- Only two types of placeholders get replaced, normal like "{{ varName }}" and custom like "<!--{{ varName }}-->"
		- Syntaxwise space characters will matter here. On one hand I wanted to avoid the slower PREG_replace() (even if more powerful),
		  on the other hand "<!--{{ varName }}-->" will be parsed/replaced, while "<!-- {{ varName }} -->" stays hidden in displayed HTML.
	*/
	public function twigSimple( $repAry, $twigTemplate, $htmlUncomment = true )
	{
		if ( $htmlUncomment ) {
			$twigTemplate = str_replace( array_map(function($s) { return '<!--{{ '.$s.' }}-->'; }, array_keys($repAry)), array_values($repAry), $twigTemplate);
		}
		$twigTemplate = str_replace( array_map(function($s) { return '{{ '.$s.' }}'; }, array_keys($repAry)), array_values($repAry), $twigTemplate);
		return preg_replace('/\{\{\s+[a-zA-Z]{1}\.\s+\}\}/','',$twigTemplate);
	}

	/*
		Will return array with found ISSNs, or FALSE if none
	*/
	public function getIssn($term = '', $atBeginn = false, $listSep = '' )
	{
		if ( $try = trim( strip_tags( urldecode($term) ) ) ) {
			if ( !$atBeginn || strpos('+1234567890',$try[0]) ) {
				$try = urldecode( str_ireplace( ['&minus;','&#8222;','&mdash;','&#8212;','&ndash;','&#8211;'], ['-','-','-','-','-','-'], rawurlencode($try) ) );
				if ( strpos($try,'--') ) {
					$try = preg_replace('/\-{2,}/','-',$try);
				}
				$ary = [];
				preg_match_all('/\b\d{4}\s*\-\s*\d{4}(\.|\b)|\b\d{4}\s*\-\s*\d{3}[Xx](\.|\b)/',$try,$ary);	// allowing e.g. '1234 - 555X' (removing spaces below then)
				if ( sizeof($ary[0]) ) {
					$ary['issn'] = array_map( function($t) { return preg_replace('/\s+/','',$t); }, array_values($ary[0]) );
					return ( empty($listSep) ? $ary['issn'] : implode( $listSep, $ary['issn'] ) );
				}
			}
		}
		return FALSE;
	}

	/*
		To clean/normalize a single ISSN
	*/
	public function cleanIssn($issn = '',$tuneFunc = 'strtoupper')
	{
		$issn = str_ireplace( array('&minus;','&#8222;','&mdash;','&#8212;','&ndash;','&#8211;'), array('-','-','-','-','-','-'), strip_tags($issn) );
		if ( $issn = trim(urldecode($issn)) ) {
			$issn = preg_replace('/\-{2,}/','-',preg_replace('/\s+/','',$issn));
		}
		if ( @!empty($tuneFunc) && function_exists($tuneFunc) ) {
			$issn = $tuneFunc($issn);
		}
		return $issn;
	}

	/*
		Bentobox creation from a Twig Template
		Note/convention:
		If $twigFile starts with '.' or '/' then let's assume this is a full path to the Twig file.
	*/
	public function makeBentobox( $htmlData, $cssStyle = '', $cssClass = 'csl-bib-body', $twigFile = 'lib4ri-bentobox-empty.twig' )
	{
		if ( strpos('./',substr($twigFile,0,1)) === false ) { // so add Tiwg directory:
			$twigFile = ( @defined( W_S_INSTALL_PATH ) ? strval( W_S_INSTALL_PATH ) : './' ) . 'twig/' . $twigFile;
		}
		if ( $twig = @file_get_contents($twigFile) ) {
			$obj = new apiQueryTools();
			if ( is_array($htmlData) ) {
				return $obj->twigSimple( $htmlData, $twig );
			}
			return $obj->twigSimple( array( 'content' => $htmlData, 'class' => $cssClass, 'style' => $cssStyle ), $twig );
		}
		return ( is_array($htmlData) ? print_r($htmlData,1) /* just as error indication */ : $htmlData );
	}
}

class apiQueryTools	/* individual/'dummy' class for traits functions, just to have them vailable independingly on other classes */
{
	use apiQueryFutureTraits;
	use apiQueryTraits;
}
