<?php

// ========================================================================
// global definitions

@include_once('./search.config.inc');


// checks happen only if following globals are not set, or true:
// $_websearch_access_check_bot = true;	// to check for bots (an exit) as soon as this files is loaded
// $_websearch_access_check_ip = true;	// to check check when this files is loaded! Do it yourself!


// global list with IP ranges in CDIR style. Usually white-listed, index with '!' will be black-listed!
// IP ranges may be defined multiple times, since the intended list-parsing function websearch_cidr_list_named()
// will eliminate duplicates before returning the requested/named CIDR list.
// Tech. note:
// Do not remove Eawag/Empa/PSI/WSL sub-arrays (or leave them empty) - they (their key) is expected to exist!
$_websearch_cidr_list = array(
	'world' => array(
		'0.0.0.0/0',
	),
	'eawag' => array(
		'152.88.128.0/17',
		'vpn'   => '152.88.189.0/24',
		'staff' => '152.88.42.0/24',
		'lan'   => '10.192.0.0/10',
	/*	'lan'   => '10.193.64.0/18',	experienced, covered above */
		'eduroam' => '172.29.0.0/16',	/* common for Eawag+Empa, remind websearch_ip_from('eawag'|'empa') checks! */
	),
	'empa' => array(
		'152.88.0.0/20',
		'152.88.16.0/21',
		'152.88.24.0/22',
		'152.88.28.0/23',
		'152.88.30.0/24',
		'152.88.32.0/21',
		'152.88.40.0/23',
		'152.88.43.0/24',
		'152.88.44.0/22',
		'152.88.48.0/20',
	/*	'vpn' => '152.88.52.0/22',	ok, but covered above */
		'152.88.64.0/18',
		'lan' => '10.128.0.0/10',
		'eduroam' => '172.29.0.0/16',	/* common for Eawag+Empa, remind websearch_ip_from('eawag'|'empa') checks! */
	),
	'psi'  => array(
		'129.129.0.0/16',
		'192.33.118.0/24',
	/*	'192.33.119.0/24',
		'192.33.120.0/21',		*/
		'vpn' => '152.88.30.0/24',
	),
	'wsl'  => array(
		'193.134.200.0/21',
		'slf' => '195.176.244.0/23',
		'vpn' => '152.88.31.0/24',
	),
	'nerd' => array(
		'fh' => '152.88.42.103/32',
		'da' => '152.88.140.161/32',
		'lk' => '152.88.30.199/32',
	),
	'dev'  => array(
		'fh' => '152.88.42.103/32',
		'fc' => '152.88.42.51/32',
	),
	'!not' => array('127.0.0.0/8'),
	'!ign' => array('255.255.255.255/32'),
);

$_websearch_bot_blacklist = array(	/* global list with term of bots (resp. user-agent string) we omit */
	'bot,search,crawl,spider,scan,scout,slurp',				/* generic */
	'curl,wget,typo3,java,python,script,rambler,zoom',		/* tools */
	'archiv,index,httrack',						/* indexers */
	'ads,job,seo',					/* other/commercials */
);


$_websearch_error_list = array(		/* see: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status	*/
	100 => 'Continue',
	101 => 'Switching Protocols',
	200 => 'OK',
	201 => 'Created',
	202 => 'Accepted',
	203 => 'Non-Authoritative Information',
	204 => 'No Content',
	205 => 'Reset Content',
	206 => 'Partial Content',
	300 => 'Multiple Choices',
	301 => 'Moved Permanently',
	302 => 'Found',
	303 => 'See Other',
	304 => 'Not Modified',
	305 => 'Use Proxy',
	307 => 'Temporary Redirect',
	400 => 'Bad Request',
	401 => 'Unauthorized',
	402 => 'Payment Required',
	403 => 'Forbidden',
	404 => 'Not Found',
	405 => 'Method Not Allowed',
	406 => 'Not Acceptable',
	407 => 'Proxy Authentication Required',
	408 => 'Request Timeout',
	409 => 'Conflict',
	410 => 'Gone',
	411 => 'Length Required',
	412 => 'Precondition Failed',
	413 => 'Payload Too Large',
	414 => 'URI Too Long',
	415 => 'Unsupported Media Type',
	416 => 'Range Not Satisfiable',
	417 => 'Expectation Failed',
	426 => 'Upgrade Required',
	500 => 'Internal Server Error',
	501 => 'Not Implemented',
	502 => 'Bad Gateway',
	503 => 'Service Unavailable',
	504 => 'Gateway Timeout',
	505 => 'HTTP Version Not Supported',
);

// ========================================================================
// global functions

/*
	Partially based on https://api.drupal.org/api/drupal/includes%21bootstrap.inc/function/ip_address/7.x
	However preferring here HTTP_X_REAL_IP if available.
*/
function websearch_ip_user_list( $sepMulti = '', $ipProxyAry = array() ) {
	if( @empty($ipProxyAry) ) {	
		// Without proxy array as input, and if there is not any specific IP assignment (by the proxy) then just return REMOTE_ADDR:
		if ( @!isset($_SERVER['HTTP_X_REAL_IP']) && @!isset($_SERVER['HTTP_CLIENT_IP']) && @!isset($_SERVER['HTTP_X_FORWARDED_FOR']) ) {
			return ( $sepMulti == '[]' ? array( $_SERVER['REMOTE_ADDR'] ) : $_SERVER['REMOTE_ADDR'] );
		}
	}
	$ipAry = ( function_exists('ip_address') /* by Drupal 7 only */ ? array( ip_address() ) : array() );
	$ipAry[] = @strval( $_SERVER['HTTP_X_REAL_IP'] );
	$ipAry[] = @strval( $_SERVER['HTTP_CLIENT_IP'] );
	$ipAry = array_merge( $ipAry, explode(',',@strtr($_SERVER['HTTP_X_FORWARDED_FOR'],';',',')) );
	$ipAry[] = @strval( $_SERVER['REMOTE_ADDR'] );
	$ipAry = array_map('trim',$ipAry);
	$ipAry = array_filter($ipAry);	// separate row/command to avoid a 'strict' PHP warning
	if ( sizeof($ipProxyAry) ) {
		$ipAry = array_diff( $ipAry, array_map('trim',$ipProxyAry) );
	}
	if ( empty($sepMulti) ) { // empty means just 1 ip is requested (not a potential list)
		return array_shift($ipAry);	// ...assuming the first IP is the best one (may not be at index/key [0] !)
	}
	$ipAry = array_unique($ipAry);
	return ( $sepMulti == '[]' ? array_values($ipAry) : implode($sepMulti,$ipAry) );
}

/*
	Converts a CIDR range notation like '152.88.0.0/16' into a min/max array( 152.88.0.0, 152.88.255.255 )
	Hint: Leaving $tuneFunc empty will return the IP range as long IPs.
*/
function websearch_cidr_range($cidr, $tuneFunc = 'long2ip') {
	$cidrAry = array_map( 'trim', explode('/', trim($cidr,"/ \r\n\t\v\x00").'/32') );
	$ipRange = array();
	$ipRange[0] = (ip2long($cidrAry[0])) & ( (-1 << (32 - intval($cidrAry[1]))) );
	$ipRange[1] = $ipRange[0] + pow(2, (32 - intval($cidrAry[1]))) - 1;
	return ( ( !empty($tuneFunc) && $tuneFunc[0] != '/' && function_exists($tuneFunc) ) ? array_map($tuneFunc,$ipRange) : $ipRange );
									   
  
				 
}

/*
	function examing the user-agent string of the client.
	Will return TRUE if idenfied as as bot.
	Needs global $_websearch_bot_blacklist to operate!
*/
function websearch_by_bot($user_agent = '', $demand_language = TRUE /* safer to insist on this, also citeProc is language-related */ ) {
	/*
	if ( @empty($_SERVER['HTTP_ROOT']) ) {
		return FALSE;		// we must be on a console!(?)
	}
	*/
	if ( $demand_language ) {
		if ( !( $tmp = @trim($_SERVER['HTTP_ACCEPT_LANGUAGE']) ) ) {	// Warning:  older PHP version do not like empty(trim(... checks!
			return TRUE;	// considered to be a bot
		}
	}
	if ( empty($user_agent) ) {
		if ( !( $user_agent = @trim($_SERVER['HTTP_USER_AGENT']) ) ) {
			return TRUE;	// with an empty user agent string we will consider the user as a bot
		}
	}

	$botAry = @array_unique( $GLOBALS['_websearch_bot_blacklist'] );

	$bot_regex = '/' . implode('|',array_map('trim',explode(',',implode(',',$botAry)))) . '|^$/i';
	if ( function_exists('variable_get') /* within Drupal!? */ ) {
		$bot_regex = trim(variable_get('lib4ri_websearch_exclude_bots_regex',$bot_regex));
	}

	return ( !empty($bot_regex) && preg_match($bot_regex, $user_agent) === 1 );
}

/*
	Helper function to split our *proprietary* combined CIDR list into black-list and white-list
	Needs GLOBAL $_websearch_cidr_list whereof (only) an index with '!' will go to the black-list!
*/
function websearch_cidr_list_named( $listName = 'black' ) {
	$cidrAllAry = $GLOBALS['_websearch_cidr_list'];
	$listName = strtolower(trim($listName));
	$cidrAry = array();		// = []; // may not be accepted by older PHP versions!(?)
	if ( $listName == 'lib4ri' ) {	// special case
		foreach( array('eawag','empa','psi','wsl') as $inst ) {
			$cidr = $cidrAllAry[$inst];
			$ary = ( is_array($cidr) ? array_values($cidr) : explode(',',strtr(trim($cidr),';',',')) );
			$cidrAry = array_merge( $cidrAry, array_combine( $ary, $ary ) );
		}
		return $cidrAry;
	}
	if ( $listName != 'black' && $listName != 'white' ) {
		$listName = ( isset($cidrAllAry[$listName]) ? $listName : 'black' );
	}
	foreach( $cidrAllAry as $idx => $cidr ) {
		if ( $listName == 'black' || $listName == 'white' ) {
			$idx = ( ( strpos($idx,'!') === false ) ? 'white' : 'black' );
		} else {	// we just want PSI's CIDR:
			if ( strtolower($idx) != $listName ) { continue; }
			$idx = $listName;
		}
		$ary = ( is_array($cidr) ? $cidr : explode(',',strtr(trim($cidr),';',',')) );
		foreach( $ary as $cidr ) {
			if ( $cidr = trim($cidr) ) { $cidrAry[$idx][$cidr] = $cidr; }
		}
	}
	if ( $listName == 'black' && @!sizeof($cidrAry['black']) ) {
		$cidrAry['black']['255.255.255.255/32'] = '255.255.255.255/32';	// Convention: ensure the blacklist is not empty!
	}
	return $cidrAry[$listName];
}

/*
	We perhaps will state more than one IP for a client
	HTTP_X_FORWARDED_FOR 'often' may have 2 or even 3.
	HTTP_X_REAL_IP may also provide an(other) IP, HTTP_CLIENT_IP as well.
	We better should not relay on REMOTE_ADDR because this only can be a proxy
*/

function websearch_ip_in_network( $ipList = null, $cidrSet = null, $ipReqAll = false, $retValueFailed = false ) {
	$ipAry = array_filter( array_map( 'trim', ( @is_array($ipList) ? $ipList : explode(',',strtr(strval($ipList),';',',')) ) ) );
	$cidrAry = array_filter( array_map( 'trim', ( @is_array($cidrSet) ? $cidrSet : explode(',',strtr(strval($cidrSet),';',',')) ) ) );
	if ( sizeof($ipAry) && sizeof($cidrAry) ) {	// insist on both!
		$ipCount = 0;
		foreach( $cidrAry as $cidr ) {
			$ipLongAry = websearch_cidr_range($cidr,'/* keep/return long IP */');
			foreach( $ipAry as $ip ) {
				$ipLong = ip2long($ip);
				if ( $ipLong >= $ipLongAry[0] && $ipLong <= $ipLongAry[1] ) {
					if ( !$ipReqAll ) {
						return true;	// IP found!
					}
					$ipCount++;
				}
			}
		}
		return ( $ipCount >= sizeof($ipAry) );		// remind that $cidrAry may have CIDR range that may overlap, so an IP can be found more than once!
	}
	return $retValueFailed; // if we have a network/cidrSet made by black-list, it may be use to return true on failure for safety reasons.
}

/*
	helper/convenience function for usual IP checks

	Needs GLOBAL variable $_websearch_cidr_list.
	Special rangeNames are 'white', 'black', 'lib4ri' - these are not keys
	in $_websearch_cidr_list, which IPs are covered is defined by the code.

	Example: websearch_ip_from('empa')
*/
function websearch_ip_from( $rangeName = 'black' ) {
	if ( $rangeName == 'white' || @is_array($GLOBALS['_websearch_cidr_list']) ) {
		if ( $rangeName == 'lib4ri' ) {
			return websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named('lib4ri') );
		}
		return websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named($rangeName), ( $rangeName == 'white' ), ( $rangeName == 'black' ) );
	}
	return websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named('black'), false, true );
}


// Offline-Test:
// echo print_r( websearch_cidr_list_named('wsl'), 1 ); exit;


// ========================================================================
// global condition:

if ( @!isset($_websearch_access_check_bot) || boolval( $_websearch_access_check_bot ) ) {
	if ( websearch_by_bot() ) {
		header("HTTP/1.0 503 Service Unavailable");
		die("503 Service Unavailable");
		exit;
	}
}
if ( @!isset($_websearch_access_check_ip) || boolval( $_websearch_access_check_ip ) === true ) {
	if ( websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named('black'), false, true ) ) {
		header("HTTP/1.0 403 Forbidden");
		die("403 Forbidden");
		exit;
	}
	if ( !websearch_ip_in_network( websearch_ip_user_list(), websearch_cidr_list_named('white'), true ) ) {
		if ( $html = @file_get_contents('/var/www/html/web/error/503.html') ) {
			header('Content-Type: text/html; charset=utf-8');
			header('Cache-Control: no-cache, max-age=300, must-revalidate');
			print( str_ireplace('Access Forbidden', 'Currently Closed', $html ) );
		} else {
			header("HTTP/1.0 503 Service Unavailable");
			die("503 Service Unavailable");
		}
		exit;
	}
}
