<?php

/*
	Wikipedia 'related list' API 
	This is intended to show result links for a given search term as a list (similar to other APIs used here)
*/
class apiQueryWikipedia extends apiQueryUtils
{
	// NOT FINAL YET - WORK IN PROGRESS!
	// Main issue:
	// The currently used/referenced API module from Wikimedia does not return the total amount of search results
	
	// see https://en.wikipedia.org/w/api.php
	// see https://www.mediawiki.org/wiki/API:Page_info_in_search_results

	public function __construct($apiLabel = '')
	{
		$this->apiName = 'wikipedia';
		$this->apiHost = 'wikimedia';
		$this->apiLabel = 'Wikipedia';		//  will serve a ID for this type of API request!
		$this->searchLimit = 10;
		$this->apiKey = '';
		$this->apiScope = '';
		parent::__construct($this->apiLabel);
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$apiUrlAry = array(
			'action' => 'query',
			'format' => 'json',
			'generator' => 'prefixsearch',
			'prop' => 'info|pageprops|pageimages|description|extracts',
			'gpssearch' => $searchTerm,
			'gpslimit' => max( min( $this->searchLimit, 30 ), 1 ) + 1,	/* +1 because of the 'may refer to' matter */
			'ppprop' => 'displaytitle',
			'inprop' => 'url',
			'redirects' => '',
			'exintro' => '',
			'explaintext' => '',
		/*	'piprop' => 'thumbnail',
			'pilimit' => max( min( $this->searchLimit, 30 ), 1 ) + 1,
			'pithumbsize' => '80',			*/
		);
		return ( 'https://en.wikipedia.org/w/api.php?' . http_build_query($apiUrlAry,'','&',PHP_QUERY_RFC3986) );
		// for example:
		// https://en.wikipedia.org/w/api.php?action=query&format=json&gpssearch=Deuterium&gpslimit=10&generator=prefixsearch&ppprop=displaytitle&redirects=&prop=info|pageprops|pageimages|description|extracts&exintro&explaintext&inprop=url
		// https://en.wikipedia.org/w/api.php?action=query&format=json&srsearch=Deuterium&list=search&srinfo=totalhits&srlimit=10&redirects=&prop=info|description&inprop=url
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		return( 'https://en.wikipedia.org/wiki/' . rawurlencode($searchTerm) );
		// for example:
		// https://en.wikipedia.org/wiki/Hydrogen
	}

	public function makeHtml($jsonData = null, $maxLength = 150, $cssItem = 'csl-entry')
	{
		$htmlAry = [];
		$jsonAry = ( is_array($jsonData) ? $jsonData : json_decode( ( is_string($jsonData) ? $jsonData : json_encode($jsonData) ), true ) );
		$max = $this->searchLimit;		// to count, because of the 'may refer to' matter
		foreach( $jsonAry['query']['pages'] as $wId => $wAry ) {
			$tmp = trim( strip_tags( urldecode( $wAry['extract'] ) ) ); // rather for safety
			if ( substr($tmp,-14) == ' may refer to:' ) {
				continue;		// special case, this is listed as normal result!(?) Link in the footer will do the same
			}
			if ( strlen($tmp) > ($maxLength + 20) ) {
				$tmp = rtrim(substr($tmp,0,$maxLength+10));
				if ( $pos = strrpos($tmp,' ') ) { $tmp = substr($tmp,0,$pos); }
				$tmp = rtrim($tmp,":;,. \\\r\t\n\v\x00") . '...';
			}
			$desc = trim( strip_tags( urldecode( $wAry['description'] ) ) ); // rather for safety
			$tmp = '<a href="' . $wAry['fullurl'] . '" target="_blank">' . $wAry['title'] . ( empty($desc) ? '' : ' ('.lcfirst($desc).')' ) . '</a>: ' . $tmp;
			$htmlAry[] = '<div class="' . $cssItem . '" style="display:inline-block;">' . $tmp . '</div>';
			 
			if ( ( --$max ) < 1 ) { break; }
		}
		return implode('',$htmlAry);
	}

	public function getNumFound($apiUrl = '')			/* no way found to get this with the inital request - to be tuned */
	{
		if ( empty($apiUrl) ) {
			$searchTerm = $this->getSearchTerm($searchTerm);
			$apiUrl = 'https://en.wikipedia.org/w/api.php?action=query&format=json&srsearch='. $searchTerm .'&list=search&srinfo=totalhits&srlimit=1&redirects=';
		}
		$jsonAry = json_decode( $this->makeQuery($apiUrl), true );
		return @intval($jsonAry['query']['searchinfo']['totalhits']);
	}
}
