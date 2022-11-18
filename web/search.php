<?php

$dirBase = '/var/www/html/web/';

/*
	make a copy of this this file is intended for:
		/var/www/html/index.php
		/var/www/html/web/index.php
		/var/www/html/web/search.php
*/


if ( !( @include_once($dirBase.'search.protect.inc') ) ) { // trivial access check - include it first!
	header('HTTP/1.0 503 Service Unavailable');
	echo '503 Service Unavailable';
	exit;
}


echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">';


if ( $html = @file_get_contents($dirBase.'search/search.header.html') ) {
	echo str_replace('lib4ri-websearch.css','lib4ri-websearch.new.css',$html);
}


echo '<body>';


if ( $html = @file_get_contents($dirBase.'search/search.navigation.html') ) {
	echo str_replace('lib4ri-websearch.css','lib4ri-websearch.new.css',$html);
}

if ( $html = @file_get_contents($dirBase.'search/drupal.node-code.html') ) {
	echo str_replace('lib4ri-websearch.css','lib4ri-websearch.new.css',$html);
}


echo '</body></html>';
?>
