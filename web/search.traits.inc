<?php

trait apiQueryTraits
{
	// unfinished, TBA
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
		return str_replace( array_map(function($s) { return '{{ '.$s.' }}'; }, array_keys($repAry)), array_values($repAry), $twigTemplate);
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

}

class apiQueryTools	/* individual/'dummy' class for traits functions, just to have them vailable independingly on other classes */
{
	use apiQueryFutureTraits;
}

