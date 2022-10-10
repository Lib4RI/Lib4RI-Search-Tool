<?php

// vvvvvvvvvvvvv GLOBAL CONFIGURATION FILE TO PROVIDE CONSTANTS vvvvvvvvvvvvvvv
// Convention: All constants will get 'W_S_' attached in front, so for example the name of a constant is intended
// to be W_S_EXAMPLE_CONST although assigning here only in $configAry 'EXAMPLE_CONST' => 'ABC' for convenience.

$configAry = array(

	'RESULT_TITLE_LINKED' => false, /* Sometimes SLSP is providing an URL for the title of a search result - should we show the URL? */
	'API_ROMEO_LIC_VER'   => true,	/* Will add version number (if available) to the license term provided by Sherpa/Romeo */ 

	'API_SLSP_KEY'     => '7777....ffff',
	'API_SCOPUS_KEY'   => 'fedc....4321',
	'API_WOS_KEY'      => 'a123....789f',
	'API_ROMEO_KEY'    => 'ddbb....5566',
	'API_CROSSREF_KEY' => 'my....@uni.org',

	'API_LIB4RI_URL'   => 'https://search.lib4ri.ch/api/',
	'API_LIB4RI_DIR'   => '/var/www/html/api/',
	
);






// vvvvvvvvvvvvvvvvv Keep at the end (to parse config array) vvvvvvvvvvvvvvvvvv
foreach( $configAry as $const => $value ) { define( 'W_S_'.$const, $value ); }
