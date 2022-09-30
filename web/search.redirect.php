<?php

/*
	Entire PHP code of this file to be added on top of 
		https://www.lib4ri.ch/site/search/search.php
		resp.
		/www/lib4ri/site/search/search.php
		
	If search.protect.inc is available and uncommented here then
	the lower/global part in that file must be deactivated.
*/

if ( @!isset($_GET['old']) /* && @include_once( './search.protect.inc') */ ) {
//	if ( function_exists('websearch_ip_from') && websearch_ip_from('lib4ri') ) {
		// Try to redirect:
		// Old Link:
		// https://www.lib4ri.ch/site/search/search.php?search=Climate+Change&x=0&y=0&type=institutes
		// New Link:
		// https://search.lib4ri.ch/?tab=1&find=Climate%20Change

		$tabAry = array(
			'articles' => 1,
			'journals' => 2,
			'books' => 1,
			'references' => 3,
			'institutes' => 3,
			'more' => 3,
		);
		$type = strtolower( strip_tags( $_GET['type'] ) );
		$urlNew = 'https://search.lib4ri.ch/';
		$urlNew .= '?tab=' . max( @intval( $tabAry[$type] ), 1 );
		$urlNew .= '&find=' . rawurlencode( strip_tags( $_GET['search'] ) );

		header('Location: ' . $urlNew );
		exit;
//	}
}

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvv FORMER/ORIG CODE vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

