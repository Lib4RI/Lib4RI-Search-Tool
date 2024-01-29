<?php

class apiQueryCurrency extends apiQueryUtils
{	/*
		We need to know the exchange rate to determine whether or not to show Lib4RI funding hint depending on the APC price.

		Currently rates are take from an UBS page, intended cage time 1h.
			https://secure.ubs.com/global/en/quotes.html
		There are ( free) APIs for this, check/overview:
			https://geekflare.com/finance/best-currency-exchange-apis/
			https://secure.ubs.com/app/ABU/4/QCoreWeb/GRT_4_Dispatch/4.0.0/pub/pg_mi/main/js/int/x/miCurrencyOverview.do	<== see source-code
		Example:
			https://www.exchangerate-api.com/	( need to register )
			https://exchangeratesapi.io/	( 250 request free per month, so cache time of >= 3h possible )
	*/

	public $request = '';
	public $exRateAry = array();
	protected $cacheTime = 5400;	// in seconds, cached currently as JSON file
	protected $jsonType = '';		// to influence how the output should be

	protected function tuneRequest($request = '')	// to turn '€' or 'E/CHF' into EUR/CHF
	{
		if ( empty($request) ) { return ''; }

		$ary = array_map('trim',explode('/',$request));
		$ary = array_map('strtoupper',array_map('urlencode',$ary));
		foreach( $ary as $idx => $cur ) {
			if ( $cur == 'DOLLAR' || $cur == 'S' || $cur == '%24' ) {
				$ary[$idx] = 'USD';
			} elseif ( $cur == 'EURO' || $cur == 'E' || $cur == '%E2%82%AC' ) {
				$ary[$idx] = 'EUR';
			} elseif ( $cur == 'POUND' || $cur == 'P' || $cur == '%C2%A3' ) {
				$ary[$idx] = 'GBP';
			} elseif ( $cur == 'YEN' || $cur == 'Y' || $cur == '%C2%A5' ) {
				$ary[$idx] = 'JPY';
			}
		}
		if ( empty($ary[1]) ) {
			$ary[1] = ( $ary[0] == 'CHF' ? 'USD' : 'CHF' );
		}
		$this->request = urldecode(implode('/',$ary));
		return $this->request;
	}
	
	public function __construct( $jsonType = '' )	// empty for file DL
	{
		$this->apiName = 'currency';
		$this->apiLabel = ( empty($apiLabel) ? 'Currency List' : $apiLabel );
		$this->jsonType = $jsonType;
		parent::__construct($this->apiLabel);
	}

	public function apiUrl($searchTerm = '', $searchOffset = -1)
	{
		return 'https://secure.ubs.com/app/ABU/4/QCoreWeb/GRT_4_Dispatch/4.0.0/pub/pg_mi/main/js/int/x/miCurrencyOverview.do';	// = portion of an HTML page
	}

	public function webUrl($searchTerm = '', $searchOffset = -1)
	{
		return 'https://secure.ubs.com/global/en/quotes.html';
	}

	public function getRateData()		// will assign $this->exRateAry
	{
		$curFile = '/tmp/currency-exchange-rates.chache.json';
		$exRateAry = array( /* exchange => array( Price | Change | Day chg. % | Low | High | 52W low | 52W high ) */ );

		if ( $this->cacheTime > 0 && @filesize($curFile) && ( time() - filemtime($curFile) ) < $this->cacheTime ) {
			$ary = @json_decode( file_get_contents($curFile), true );
			if ( is_array($ary) && sizeof($ary) > 3 ) {
				$this->exRateAry = $ary;
				return;
			}
		}
		$browser = strval( ( time() >> 23  ) - 97 );	// assuming all 3 months a new version
		$browser = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:'.$browser.'.0) Gecko/20100101 Firefox/'.$browser.'.0';
		$htmlCode = $this->makeQuery( $this->apiUrl(), 10, $browser );

		if ( $pos = strpos($htmlCode,'aceq_table') ) {
			$htmlCode = substr(strchr(substr($htmlCode,$pos,strpos($htmlCode,'</table>')-$pos),'>'),1);
			foreach( explode('</tr>',$htmlCode) as $trArea ) {
				$trAry = array_map('strip_tags',explode('</td>',$trArea));
				if ( strpos($trAry[0],'/') ) {
					$exRateAry[array_shift($trAry)] = array_slice($trAry,0,7);
				/*	$exRateAry to hold e.g. :		[EUR/CHF] => Array(
														[0] => 1.0156
														[1] => -0.02274
														[2] => &minus;2.19%
														[3] => 1.01308
														[4] => 1.04187
														[5] => 0.99728
														[6] => 1.09913
													)							*/
				}
			}
			if ( sizeof($exRateAry) > 3 ) {
				file_put_contents($curFile,json_encode($exRateAry,JSON_PRETTY_PRINT));
			}
		}
		if ( sizeof($exRateAry) > 3 ) {	$this->exRateAry = $exRateAry; }
	}
	
	public function queryNow($request = /* 'USD/CHF' */ '' )	// note with 'USD' only you will just get the rate as number
	{
		if ( empty($request) ) {
			$this->setupParam();
            if ( !empty($this->searchTerm) ) {
				if ( strtoupper($this->searchTerm) == 'CHF' ) {
					return 1.0;
				}
				$request = $this->searchTerm;
			}
		}
		$this->getRateData();
		
		$this->tuneRequest( $request . ( @strpos($request,'/') ? '' : ( empty($request) ? '' : '/CHF' ) ) );	// will update $this->request
	
		if ( empty(trim($request)) || !isset($this->exRateAry[$this->request]) ) {
			return $this->jsonOutput( $this->exRateAry, $this->jsonType );
		}
		if ( $this->jsonType == 'float' || $this->jsonType == 'double' ) { // special case
			return array_shift($this->exRateAry[$this->request]);		// intentionally no JSON, same as $this->jsonOutput( ARRAY/DATA, 'none' )
		} 
		return $this->jsonOutput( array( $this->request => $this->exRateAry[$this->request] ), $this->jsonType );
	}
}
