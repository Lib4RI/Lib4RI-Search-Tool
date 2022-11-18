<?php

include_once('./meta-remap.citeproc.inc');		// for provide the *essential* $MetaMap array();

/*
	function to return an array with the markup-tuning variales+functions for citeproc.
	the api-specific functions fprm these variables are define in meta-remap.citeproc.inc
*/
function websearch_citeproc_polish_markup( $apiName ) {
	$citeProcMarkup = array();
	if ( @!empty($apiName) && @isset($GLOBALS['metaMap']['citeproc'][$apiName]) ) {
		foreach( $GLOBALS['metaMap']['citeproc'][$apiName] as $fieldName => $mapped ) {
			if ( substr($fieldName,0,1) != '_' /* with '_' it is auxiliary, no real metadata field */ ) {
				$funcName = 'websearch_citeproc_' . $apiName . '_polish_' . strtolower($fieldName);
				if ( @isset($GLOBALS[$funcName]) && is_callable($GLOBALS[$funcName],true) ) {
					$citeProcMarkup[$fieldName] = $GLOBALS[$funcName];
				}
			}
		}
	}
	return $citeProcMarkup;
}

/*
	loopable helper function to post-process CiteProc data array, untested
*/
function websearch_citeproc_polish_rendered( $apiName, $fieldName /* the parent/top *index* as in metaMap array */, $fieldValue ) {
	$funcName = 'websearch_citeproc_' . $apiName . '_polish_' . strtolower($fieldName);
	return ( ( @isset($GLOBALS[$funcName]) && is_callable($GLOBALS[$funcName],true) ) ? $GLOBALS[$funcName]($fieldValue) : $fieldValue );
}

/*
	loopable helper function to modify metadata as got from the API, untested
*/
function websearch_citeproc_prepare_retrieved( $apiName, $fieldName /* the parent/top *index* as in metaMap array */, $fieldValue, $argAry = array() ) {
	$funcName = 'websearch_citeproc_' . $apiName . '_prepare_' . strtolower($fieldName);
	return ( ( @isset($GLOBALS[$funcName]) && is_callable($GLOBALS[$funcName],true) ) ? $GLOBALS[$funcName]($fieldValue,$argAry) : $fieldValue );
}

/*
	helper function to check/tune the syntax of our 'meta-path' as used in meta-remap.citeproc.inc
	It needs to be specified whether or not this should be done for arrays or objects.
*/
function websearch_json_path_check( $metaPath, $objType = 'array|object' ) {
	if ( stripos($objType,'a') === 0 ) {
		$ary = explode('[',strtr($metaPath,'>','['));
		foreach($ary as $idx => $prop ) {
			$prop = trim($prop,"'\"[-] \n\r\t\v\x00");		// bascially to remove potential ']'
			$quote = ( strval(intval($prop)) == $prop ? '' : '"' );
			$ary[$idx] = $quote . $prop . $quote;
		}
		return ( '[' . implode('][',$ary) . ']' );
	}
	if ( stripos($objType,'o') === 0 ) {
		$ary = explode('->',$metaPath);		// for non-alphanumeric props we need e.g.: {'search-results'}->{'results:total'}
		foreach($ary as $idx => $prop ) {
			if ( !strpos($prop,'}') && preg_match('/[^\w\d]/',$prop) && !preg_match('/\[\d+\]/',$prop) ) {
				$ary[$idx] = "{'" . trim($prop,"'\" \n\r\t\v\x00") . "'}";
			}
		}
		return ( '->' . implode('->',$ary) );
	}
	return $metaPath;
}


/*	
	function to transform meta data from a given API
	into a json with a structure CiteProc can handle.
*/
function websearch_json_remap( $jsonFromApi /* Json-Code or Object */, $apiName, $goalFromat = 'citeproc', $returnMode = 0 ) {

	if ( @!isset($GLOBALS['metaMap'][$goalFromat][$apiName]) /* API-specific $metaMap array in/from citeproc.meta-remap.inc */ ) { return FALSE; }
	$metaMapApi = $GLOBALS['metaMap'][$goalFromat][$apiName];
	if ( @!is_array($metaMapApi) /* API-specific $metaMap array in/from citeproc.meta-remap.inc */ ) { return FALSE; }

	$jsonFromApiParent = ( !is_string($jsonFromApi) ? $jsonFromApi : json_decode($jsonFromApi,true) );
	
	if ( $metaPath = @trim($metaMapApi['_parent_node']) ) {
		$metaPath = websearch_json_path_check( $metaPath, gettype($jsonFromApiParent) );
		eval('if ( @( $jsonFromApiParent'.$metaPath.' !== null ) ) { $jsonFromApiParent = $jsonFromApiParent'.$metaPath.'; }');
	}
	
	foreach( $metaMapApi as $idx => $metaPath ) {
		// clean up ouir copy of $metaMap from auxiliary paths/values:
		if ( strpos($idx,'_') === 0 ) { unset($metaMapApi[$idx]) /* not needed anymore */ ; }
	}

	// echo "\r\n" . print_r($jsonFromApiParent,1) . "\r\n"; return;

	$citeProcAry = array();
	foreach( $jsonFromApiParent as $dPubIdx => $dPubFound ) {

		// Let's go through our mapping array. Main cases are detectable by analysing the structure
		// of the mapping array, so far however they are often just identified via $mIdx value.

		foreach( $metaMapApi as $mIdx => $mapData ) {

			if ( @!is_array($mapData) ) {
				foreach( array_map('trim',explode('|',$mapData)) as $mData ) {
					$metaPath = websearch_json_path_check( $mData, gettype($dPubFound) );
					$eVal = -2;
					eval('$eVal = ( @( $dPubFound'.$metaPath.' !== null ) ? strval($dPubFound'.$metaPath.') : "" );');
					if ( $eVal != "" ) {
						$eVal = websearch_citeproc_prepare_retrieved( $apiName, $mIdx, $eVal );
						$citeProcAry[$dPubIdx][$mIdx] = $eVal;
						break;
					}
				}
				continue;
			}

			// transfer from single field to array
			if ( $mIdx == 'issued' ) {
				foreach( $mapData as $dIdx => $dAry ) {	// $dIdx: 'date-parts'
					foreach( $dAry as $idx => $metaPathList ) {
						$isOK = false;
						foreach( array_map('trim',explode('|',$metaPathList)) as $metaPath ) {
							$metaPath = websearch_json_path_check( $metaPath, gettype($dPubFound) );
							$eVal = -3;
							eval('$eVal = ( @( $dPubFound'.$metaPath.' !== null ) ? strval($dPubFound'.$metaPath.') : "" );');
							if ( $eVal != "" ) {
								$eVal = websearch_citeproc_prepare_retrieved( $apiName, 'issued', $eVal );
								$eVal = str_replace( array('[',']'), array('',''), $eVal );
								$ary = explode('-',strtr($eVal,'/:','--'));		// default
								if ( preg_match('/^\d{4}\s*\-\s*\d{4}$/',trim($eVal)) ) {	// e.g. "2004-2012" is not directly PHP-processable
									$ary = array( trim(strchr($eVal,'-'),"- \n\r\n\v\x00") );
								}
								// specially for SLSP/Primo the look of the date format may vary a lot, until being incompatible for citeproc
								if ( @strtotime(implode(' ',$ary)) ) {		// recheck, can we parse it with PHP:
									$isOK = true;
								} elseif ( preg_match('/[\-|\/|\s+]\d{4}$/',' '.$eVal,$ary) || preg_match('/[\-|\/|\s+]\d{4}/',' '.$eVal,$ary) ) {
									$ary[0] = ltrim($ary[0],"/- \n\r\n\v\x00");
									$isOK = true;
								} else {
									$ary = array( '3999' /* to indicate an error */ );
								}
								$citeProcAry[$dPubIdx][$mIdx][$dIdx][$idx] = array_filter($ary);
							}
						}
						if ( $isOK ) { break; }
					}
				}
				continue;
			}
			// transfer from array to array transfer:
			if ( $mIdx == 'author' ) {
				$autCount = 0;
				while( $autCount >= 0 ) {
					$autRepeat = 0;
					foreach( $mapData[0] as $fgIdx /* = familiy, given */ => $fgPathList) {
						foreach( array_map('trim',explode('|',$fgPathList)) as $fgPathOrig ) {
							$metaPath = websearch_json_path_check( $fgPathOrig, gettype($dPubFound) );
							if ( strpos($fgPathOrig,'[0]') !== false ) {
								$autRepeat = 99;	// arbitrary safety limit
								$metaPath = str_replace('[0]','['.$autCount.']',$metaPath);		// iterate by replacing [0]
							}
							$eVal = 0;
							eval('$eVal = ( @( $dPubFound'.$metaPath.' !== null ) ? trim($dPubFound'.$metaPath.') : "" );');
							if ( !empty($eVal) ) {
								$eVal = websearch_citeproc_prepare_retrieved( $apiName, 'author', $eVal, array($fgIdx) );
								$citeProcAry[$dPubIdx][$mIdx][$autCount][$fgIdx] = $eVal;
								break;
							}
						}
					}
					$autCount = ( ( $autCount < $autRepeat && @!empty($citeProcAry[$dPubIdx][$mIdx][$autCount]['family']) ) ? ( $autCount + 1 ) : -1 );
				}
			}
		}
	}

	return ( ( $returnMode < 0 ) ? $citeProcAry : json_encode( $citeProcAry, ( $returnMode ? JSON_PRETTY_PRINT : 0 ) ) );
}



/*
	function to find the number of results found
	APIs may use differnet 'locations' for this number, hence we need a map to find it.
*/
function websearch_json_value_by_path( $jsonFromApi, $metaPath ) {

	if ( $jsonFromApi == NULL /* json code/object from API, we need data in $metaMap */ ) { return FALSE; }
	if ( empty($metaPath) /* as we have it e.g. in meta-remap.citeproc.inc */ ) { return FALSE; }

	if ( is_string($jsonFromApi) ) {
		$jsonFromApi = json_decode($jsonFromApi,true);
	}
	if ( is_array($jsonFromApi) && substr($metaPath,0,1) != '[' ) {		// to be tuned/improved!(?)
		$metaPath = websearch_json_path_check($metaPath,'array');
	}
	elseif ( is_object($jsonFromApi) && substr($metaPath,0,2) != '->' ) {
		$metaPath = websearch_json_path_check($metaPath,'object');
	}
	$eVal = -1;
	eval('$eVal = ( @( $jsonFromApi'.$metaPath.' !== null ) ? strval($jsonFromApi'.$metaPath.') : -1 );');
	return $eVal;
}
