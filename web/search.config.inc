<?php

// vvvvvvvvvvvvv GLOBAL CONFIGURATION FILE TO PROVIDE CONSTANTS vvvvvvvvvvvvvvv
// Convention: All constants will get 'W_S_' attached in front, so for example the name of a constant is intended
// to be W_S_EXAMPLE_CONST although assigning here only in $configAry 'EXAMPLE_CONST' => 'ABC' for convenience.

$configAry = array(

	'INSTALL_PATH'     => $_SERVER['DOCUMENT_ROOT'] . '/web/',	/* local path of the API libraries, may be empty */

	'RESULT_TITLE_LINKED' => false, /* Sometimes SLSP is providing an URL for the title of a search result - should we show the URL? */
	'API_ROMEO_LIC_VER'   => true,	/* Will add version number (if available) to the license term provided by Sherpa/Romeo */ 

	'API_SLSP_KEY'     => '7777....ffff',
	'API_SCOPUS_KEY'   => 'fedc....4321',
	'API_WOS_KEY'      => 'a123....789f',
	'API_ROMEO_KEY'    => 'ddbb....5566',
	'API_CROSSREF_KEY' => 'my....@uni.org',

	'API_LIB4RI_URL'   => 'https://search.lib4ri.ch/api/',
	'API_LIB4RI_DIR'   => '/var/www/html/api/',

	'HTML_TAGS_OK'     => 'b,strong,i,em,sub,sup,small,span,cite,blockquote,mark,del,ins,ul,ol,li,dl,dt,dd,p,br' /* a|code */ ,

	/* CiteProc: Define the transformation CSL file (without *.csl): */
	'CITEPROC_STYLE'   => 'national-institute-of-health-research',
	/* CiteProc's loadStyleSheet() wants *.csl file in self::vendorPath()."/citation-style-language/styles-distribution/" */
	/* Generally any upload feature would be nice to upload our own CSL file... */

);






// vvvvvvvvvvvvvvvvv Keep at the end (to parse config array) vvvvvvvvvvvvvvvvvv
foreach( $configAry as $const => $value ) { define( 'W_S_'.$const, $value ); }
