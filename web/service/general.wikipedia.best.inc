<?php

/*
	Wikipedia 'Best Hit' API 
	Intended to present 1 result by composing the HTML/content take from the best/top search result found.
*/
class apiQueryWikihit extends apiQueryUtils
{
	/* Main issue:
		- Search term is evaluated by the API in case-sensitive manner.
		- Wikipedia automatically applies some normalization on the serach term - it may not be sufficent though
	So some kind of two-step API would be more appropriate:
		- one step to find the right/best title: https://en.wikipedia.org/w/api.php?&action=opensearch&search=water%20research
		- 2nd step to find the meta data for this title: https://en.wikipedia.org/w/api.php?action=query&list=search&srsearch=water%20research
	For the moment we relay on the 'case sentive' dominance and are upper-casing the search term the user has entered
	*/
	
	public $numFound;
	public $apiUrlList;
	public $wikiList;	// intended to be array, but do NOT initate it! Example: https://en.wikipedia.org/w/api.php?&action=opensearch&search=Functional%20Ecology
	public $imgData;	// data of the thumbnail: array ( 'url' => '', 'width' => 0, 'height' => 0 )

	public function __construct($apiLabel = '')
	{
		$this->apiName = 'wikihit';		// 'hit' in sense of 'best suggestion'
		$this->apiHost = 'wikimedia';
		$this->apiLabel = 'Wikipedia';		//  will serve a ID for this type of API request!
		$this->apiScope = '';
		$this->apiUrlList = 'https://en.wikipedia.org/w/api.php?&action=opensearch&search=';	
		parent::__construct($this->apiLabel);
	}

	public function getList($searchTerm = '')
	{
		if ( empty($searchTerm) ) {
			$this->setupParam();		// refresh!
			$searchTerm = ( empty($this->searchTermOrig) ? 'rainbow' : $this->searchTermOrig );
		} else {
			$searchTerm = urldecode(rawurldecode($searchTerm)); // try to get rid if any encodings!
		}

		if ( @!empty($this->wikiList[0]) /* always underscore for space */ && strtolower(strtr($this->wikiList[0],'_',' ')) == strtolower($searchTerm) ) {
			return $this->wikiList;		// deliver the cached one!
		}

		$this->wikiList = json_decode( $this->makeQuery( $this->apiUrlList . rawurlencode(ucwords($searchTerm)) ) ); // better hits with uc-words!(?)
		/* What we get/return is an Array (
				[0] => Functional Ecology
				[1] => Array (
						[0] => Functional ecology
						[1] => Functional Ecology (journal)
					)
				[2] => Array (
						[0] => 
						[1] => 
					)
				[3] => Array (
						[0] => https://en.wikipedia.org/wiki/Functional_ecology
						[1] => https://en.wikipedia.org/wiki/Functional_Ecology_(journal)
					)
				)
		*/
		if ( ( $tmp = @strtr($this->wikiList[1][0],' ','_') ) && ( $pos = strpos($tmp,'_-_') ) ) {
			$searchTerm = substr($tmp,0,$pos+1) . '(' . substr($tmp,$pos+3) . ')';
			$this->wikiList = json_decode( $this->makeQuery( $this->apiUrlList . rawurlencode(ucwords($searchTerm)) ) ); // better hits with uc-words!(?)
		}
		return $this->wikiList;
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1) // for example: https://en.wikipedia.org/api/rest_v1/page/summary/Water_Research
	{
		$jsonAry = $this->getList($searchTerm);
		return ( 'https://en.wikipedia.org/api/rest_v1/page/summary/' . @rawurlencode(strtr(ucwords($jsonAry[1][0]),' ','_')) );
	}

	public function webUrl($searchTerm = '', $searchOffset = -1) // for example: https://en.wikipedia.org/wiki/Water_Research
	{
		$jsonAry = $this->getList($searchTerm);
		return @strval($jsonAry[3][0]);
	}

	public function getNumFound()		// of the realted results, only call after queryNow() !
	{
		return $this->numFound;
	}

	public function queryNow($searchTerm = '')
	{
		if ( empty($searchTerm) ) { $searchTerm = $this->getSearchTerm($searchTerm); }
		$apiUrl = $this->apiUrl( empty($searchTerm) ? 'dreamland' : $searchTerm );
		$jsonAry = $this->getList($searchTerm);
		$jsonData = $this->makeQuery( $apiUrl );
		$this->numFound = ( @empty($jsonAry[1][0]) ? 0 : 1 );
		return $jsonData;
	}

	public function makeHtml($searchTerm = '', $imgHeight = 200, $langAry = ['en','de','fr','it'] )
	{
		$tmpAry = json_decode($this->queryNow($searchTerm),true);
		$html = '';
		if ( @!empty($tmpAry['extract']) ) {
			if ( $imgHeight != 0 && ( $html = @trim($tmpAry['thumbnail']['source']) ) ) {
				$this->imgData = array(
					'url' => $html,
					'width' => $tmpAry['thumbnail']['width'],
					'height' => $tmpAry['thumbnail']['height'],
				);
				if ( $imgHeight > 0 ) {	 // so unchecked/unchanged with -1
					if ( $tmpAry['thumbnail']['width'] > $tmpAry['thumbnail']['height'] ) {
						$imgRatio = $tmpAry['thumbnail']['width'] / $tmpAry['thumbnail']['height'];
						$this->imgData['width'] = $this->imgData['height'];
						$this->imgData['height'] = $this->imgData['height'] / $imgRatio;
					}
					if ( $this->imgData['height'] > $imgHeight ) {
						$imgRatio = $this->imgData['height'] / $imgHeight;
						$this->imgData['width'] = $this->imgData['width'] / $imgRatio;
						$this->imgData['height'] = $imgHeight;
					}
				}
				$html = '<img src="' . $html . '" style="height:' . $this->imgData['height'] . 'px;" height="' . $this->imgData['height'] . 'px" />';
				$html = '<a href="' . $tmpAry['content_urls']['desktop']['page'] . '" target="_blank" style="float:right; margin:0 0 3px 1ex;">' . $html . '</a> ';
			}
			$ext = implode(' ',array_slice(explode(' ',$tmpAry['extract_html']),0,75));
			if ( $pos = strpos($ext,'</b>') ) {
				$ext = substr($ext,0,$pos+4) . '</a>' . substr($ext,$pos+4);
				if ( ( $pos = strpos($ext,'<b') ) !== false ) {
					$ext = substr($ext,0,$pos) . '<a href="' . $tmpAry['content_urls']['desktop']['page'] . '" target="_blank">' . substr($ext,$pos);
				}
			}
			if ( strlen($tmpAry['extract_html']) > strlen($ext) ) { $ext .= '...'; }

			$html = ( ( stripos($ext,'<p>') === 0 ) ? '<p>'.$html.substr($ext,3) : $html.$ext );
			$html .= ( ( substr($html,-2) == 'p>' ) ? '<br>' : '<br><br>' );

			array_shift($langAry);	// remove 'en'
		}
		foreach( $langAry as $idx => $lang ) {
			$html .= ( empty($idx) ? 'Wikipedia &nbsp;' : ' | ' );
			$html .= '<a href="https://www.wikipedia.org/search-redirect.php?search=' . strtr(ucWords($searchTerm),' ','_') . '&language=' . $lang . '&go=++%E2%86%92++&go=Go">';
			$html .= strtoupper($lang) . '</a>';
		}
		return str_replace('<a ','<a target="_blank" ',$html);	// enforce new page everywhere (current standard with us)
	}
	
}
