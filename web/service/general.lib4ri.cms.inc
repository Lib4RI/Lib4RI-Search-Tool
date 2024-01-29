<?php

/*
	LIB4RI *CMS* API
*/
class apiQueryLib4riCMS extends apiQueryUtils
{
	// See e-Mail from Bogdan Racz <bogdan.racz@dreamproduction.com> / Wednesday, 25 January 2023 12:51
	
	public $webHost;
	public $groupList;	// which groups are allowed to see the content
	public $linkIntern = false;		// HTML Linking: internal (or for/to new tab)?

	public function __construct($apiLabel = '')
	{
		$this->apiName = 'lib4ricms';
		$this->apiHost = 'lib4ricms';
		$this->apiLabel = 'Lib4RI Search';		//  will serve a ID for this type of API request!
		$this->searchLimit = 100;
		$this->apiKey = '';
		$this->apiScope = '';
		parent::__construct($this->apiLabel);
		
	//	$this->webHost = 'https://lib4ri.dpstage.ch'; // empty to use current domain
		$this->webHost = ( @isset($_SERVER['HTTP_REFERER']) ? rtrim( $_SERVER['HTTP_REFERER'], '/' ) : 'http://'.$_SERVER['HTTP_HOST'] );

		$this->groupList = ''; // = 'devlan,dev,lib4ri,dreamprod'; // empty for now reatriction
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		// for example: https://lib4ri.dpstage.ch/search-api.json/"latex%20training"
		return ( $this->webHost . '/search-api.json/' . rawurlencode( empty($searchTerm) ? $this->getSearchTerm($searchTerm) : $searchTerm ) );
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		// to be implented
		return ( $this->webHost . '/search-tools/' . rawurlencode( empty($searchTerm) ? $this->getSearchTerm($searchTerm) : $searchTerm ) );
	}

	public function makeHtml($jsonData = null, $maxLength = 150, $cssItem = 'csl-entry')
	{
		// $this->makeHtml( $this->queryNow($searchTerm), -1 )
		$htmlAry = [];
		$jsonAry = ( is_array($jsonData) ? $jsonData : json_decode( ( is_string($jsonData) ? $jsonData : json_encode($jsonData) ), true ) );
		$max = $this->searchLimit;		// to count, because of the 'may refer to' matter
		foreach( $jsonAry as $resIdx => $resAry ) {
			if ( !( $tmp = $this->cleanTerm( $resAry['title'] ) ) ) { continue; }
			$url = $this->webHost . strip_tags($resAry['search_api_url']);
			$tmp = '<a href="' . $url . '"' .  ( $this->linkIntern ? '' : 'target="_blank"' ) . '>' . $tmp . '</a>';
			if ( $maxLength >= 0 ) {
				$desc = $this->cleanTerm( $resAry['search_api_excerpt'] ); // rather for safety
				if ( strlen($desc) > ($maxLength + 20) ) {
					$desc = rtrim(substr($desc,0,$maxLength+10));
					if ( $pos = strrpos($desc,' ') ) { $desc = substr($desc,0,$pos); }
					$desc = rtrim($desc,":;,. \\\r\t\n\v\x00") . '...';
				}
				$tmp .= '<br>' . $desc;
			}
			$htmlAry[] = '<div class="' . $cssItem . '" style="display:inline-block;">' . $tmp . '</div>';
			if ( ( --$max ) < 1 ) { break; }
		}
		return implode('',$htmlAry);
	}

	public function getNumFound($searchTerm = '')			/* no way found to get this with the inital request - to be tuned */
	{
		$jsonData = $this->queryNow($searchTerm);
		return ( sizeof($jsonData) );
	}
}
