<?php

class searchHandler
{
	/*
		Auxiliary link resolver for SLSP
		see:
		https://knowledge.exlibrisgroup.com/Alma/Knowledge_Articles/Troubleshooting_Tips_for_Linking_Using_the_CTO 
		https://knowledge.exlibrisgroup.com/Alma/Product_Documentation/010Alma_Online_Help_(English)/Electronic_Resource_Management/051Link_Resolver/030Access_to_Services#Troubleshooting_-_Using_the_OpenURL_Link_Resolver_Debugger_Tool 

		What it does, how it works:
			Enter e.g. Alma/MMS-ID 99116717651105522 in:
				https://slsp-lib4ri.alma.exlibrisgroup.com/view/uresolver/41SLSP_LIB/openurl?&svc_dat=CTO&debug=true&u.ignore_date_coverage=true&rft.mms_id=99116717651105522&rfr_id=info:sid/primo.exlibrisgroup.com&svc_dat=viewit&u.ignore_af_filter=true&internalAccess=true
			...then get <context_service service_type="getFullTxt" context_service_id="2237187660005522"> - NOT: <key id="ctx_id">2237244660005522</key> !!!
				https://lib4ri.swisscovery.slsp.ch/view/action/uresolver.do?operation=resolveService&package_service_id=2237187660005522&institutionId=5522&customerId=5500&VE=true
			Currently however we are going to use the content of <resolution_url> (as found in the slsp-lib4ri.alma... link) which should (always) match the composed link above!(?)

		$apiTest = new searchHandler();
		$apiTest->urlResolve('alma',99116717651105522);
	*/
	function urlResolve( $source = '', $ident = null, $redirect = true ) 
	{
		if ( empty($source) ) {
			_lib4ri_exitByError('ERROR: No pleasant subject to resolve!');
		}
		if ( !intval($ident) ) {	// assuming all IDs start with a number!
			if ( !( ( $ident = strip_tags(urldecode($_GET['id'])) ) && intval($ident) ) ) {
				if ( !( ( $ident = @strval($this->getSearchTerm($searchTerm)) /* using $_GET['find'] */ ) && intval($ident) ) ) {
					$tmp = 'ERROR: No pleasnt ID to look-up in ' . strtoupper($source) . ' specified!';
					return ( $redirect ? _lib4ri_exitByError($tmp) : $tmp );
				}
			}
		}

		if ( strtolower($source) == 'alma' ) {		// ident should be e.g. 99116717651105522
			$url = 'https://slsp-lib4ri.alma.exlibrisgroup.com/view/uresolver/41SLSP_LIB/openurl?rft.mms_id=' . $ident;
			$url .= '&svc_dat=CTO&svc_dat=viewit&debug=true&u.ignore_date_coverage=true&u.ignore_af_filter=true&internalAccess=true';
			$apiUtil = new apiQueryUtils('ResolveAlmaId');
			$apiData = html_entity_decode( $apiUtil->makeQuery( $url ) );
			$posA = strpos($apiData,'<resolution_url>') + 16;
			$posZ = strpos($apiData,'</resolution_url>',$posA);
			if ( $posZ && ( $url = @trim(substr($apiData,$posA,$posZ-$posA)) ) && strpos($url,'http') === 0 ) {
				if ( !$redirect ) { return $url; }
				header('Location: ' . $url );
				exit;
			}
			$tmp = 'ERROR: No resolving URL found for given ' . strtoupper($source) . ' ID!';
			return ( $redirect ? _lib4ri_exitByError($tmp) : $tmp );
		}

		if ( strtolower($source) == 'dora' ) {		// ident can be from different online services, all however expected to start with a number
			$solrAry = array(
				'doi' => 'mods_identifier_doi_mlt',		/* 10.1073/pnas.1619147114 */
				'wos' => 'mods_identifier_ut_mlt',		/* 000398159000051 */
				'scopus' => 'mods_identifier_scopus_mlt',	/* 2-s2.0-85017008914, perhaps without 2-s...  */
				'pubmed' => 'mods_identifier_pmid_mlt',		/* 28320947	*/
			);
			$key = '';
			if ( substr($ident,0,3) == '10.' ) {
				$key = 'doi';
			} elseif ( $len = strlen(strrchr('-'.$ident,'-')) ) {
				if ( $len > 13 ) { $key = 'wos'; }
				elseif ( $len > 10 ) { $key = 'scopus'; }
				elseif ( $len > 7 ) { $key = 'pubmed'; }
			}
			if ( empty($key) || @empty($solrAry[$key]) ) {
				$tmp = 'ERROR: Unble to auto-detect ID type!';
				return ( $redirect ? _lib4ri_exitByError($tmp) : $tmp );
			}
			$url = 'http://lib-dora-prod1.emp-eaw.ch:8080/solr/collection1/select?q=' . $solrAry[$key] . ':' . str_replace('/','%5c%2f',$ident);
			$url .= '&sort=PID+asc&rows=987654321&fl=PID%2c+' . implode('%2c+',$solrAry) . '%2c+dc.title&sort=PID+asc&rows=987654321&wt=json&indent=false';
			$apiUtil = new apiQueryUtils('IdCheckDORA');
			$apiData = $apiUtil->makeQuery( $url );
			$jsonAry = json_decode( $apiData, true );
			if ( @!isset($jsonAry['response']['numFound']) ) {
				header('Content-Type: ' . 'application/json' . ' charset=utf-8');
				print( json_encode( $jsonAry, JSON_PRETTY_PRINT ) );
			} elseif ( $jsonAry['response']['numFound'] > 1 ) {
				header('Content-Type: ' . 'application/json' . ' charset=utf-8');
				print( json_encode( $jsonAry['response']['docs'], JSON_PRETTY_PRINT ) );
			} else {
				$pid = $jsonAry['response']['docs'][0]['PID'];
				$url = 'https://www.lib4ri.ch/' . strtok(strtr($pid,'-',':'),':') . '/islandora/object/' . $pid;
				if ( !$redirect ) { return $url; }
				header('Location: ' . $url );
			}
			exit;
		}
	}

	function getJsonLinkSet( $linkSet = '', $returnMode = '' /* 'array' or 'string', otherwise a file header is send */ )
	{
		@include_once($GLOBALS['_dirAry']['local'].'frame-config.php'); // providing $arrEnginges
		$retAry = array( 'set' => array(), 'html' => '' );
		$tuneAry = array(
			'Journals at Lib4RI' => ''	/* not to show at all */ ,
			'Journals in swisscovery' => 'Journals in swisscovery CH',
		);
		if ( @isset($arrEnginges) && is_array($arrEnginges) ) {
			$searchTerm = @trim(strip_tags(rawurldecode($_GET['find'])));		// OK/standable if empty
			$linkAry = array_map('trim',explode(';',$linkSet));
			foreach($linkAry as $linkPath ) {
				if ( empty($linkPath) ) {
					$retAry['set'][] = '&nbsp;';
					$retAry['html'] .= '<div class="lib4ri-bentobox-linkset-link">&nbsp;</div>';
					continue;
				}
				$pAry = array_map('trim',explode(':',$linkPath));
				if ( sizeof($pAry) == 1 ) {
					$pAry[1] = 'main';
					$pAry[2] = '*';
				} elseif ( sizeof($pAry) == 2 ) {
					$pAry[2] = $pAry[1];
					$pAry[1] = 'main';
				}
				if ( @empty($pAry[2]) ) {	// = name of the search site
					continue;
				}
				if ( @!is_array($arrEnginges[($pAry[0])][($pAry[1])]) ) {
					continue;
				}
				$siteAry = ( strpos($pAry[2],'*') === 0 ) ? $arrEnginges[($pAry[0])][($pAry[1])] : array( $pAry[2] => $arrEnginges[($pAry[0])][($pAry[1])][($pAry[2])] );
				foreach( $siteAry as $siteName => $siteData ) {
					if ( @isset($tuneAry[$siteName]) ) {
						if ( !( $siteName = @trim($tuneAry[$siteName]) ) ) { continue; }
					}
					foreach( array('urlOrig','urlReal','url') as $urlKey ) {
						if ( $aTmp = @trim($siteData[$urlKey]) ) {
							if ( strpos($aTmp,'/') === 0 ) {
								$aTmp = 'https://www.lib4ri.ch' . $aTmp;		// add host - to be tuned...
							}
							$aTmp = str_replace('###',rawurlencode($searchTerm),$aTmp);	// to get final search link, see frame-confg.php
							$aTmp = '<a'
								. ' href="' . $aTmp . '"'
								. ' title="Search' . ( empty($searchTerm) ? '' : ' \''.$searchTerm.'\'' ) . ' at ' . $siteName . '"'
								. ' target="_blank"'
								. '>' . $siteName . '</a>';
							$retAry['set'][] = $aTmp;
							$retAry['html'] .= ( empty($retAry['html']) ? '' : "\r\n" ) . '<div class="lib4ri-bentobox-linkset-link">' . $aTmp . '</div>';
							break;
						}
					}
				}
			}
		}
		if ( empty($retAry['html']) ) {
			_lib4ri_exitByError( 'ERROR LinkSet: Something went wrong for link-set: ' . $linkSet );
		}
		/*
		if ( stripos($linkSet,'Google') && stripos($linkSet,'Lib4RI') ) { // temp. exception: 'unter construction' for internal/lib4ri.ch search:
			$htmlAry = array(
				'<table style="border:0; margin:2ex 0 2ex 0; padding:0;"><tbody><tr><td style="margin-right:2ex;">',
				'<img src="https://svgsilh.com/png-512/150271-3f51b5.png" width="256" height="238" style="width:256px; height:238px;">',
				'<!-- image source: https://wordpress.org/openverse/image/b18cfb7f-58ee-4108-b935-7c3fb9d79ce5/ -->',
				'</td><td>&nbsp;</td><td style="vertical-align:top;">',
				'<br>With the relaunch of our website, you will see a a results list here. <br>Until then, please use this customized ',
				str_replace( substr(strrchr($linkSet,':'),1), 'Google link', $retAry['set'][0] ),
				' to search lib4ri.ch.</td></tr></tbody></table>',
			);
			$retAry = array( 'set' => array( implode('',$htmlAry) ) );
			$retAry['html'] = '<div class="lib4ri-bentobox-linkset-link">' . $retAry['set'][0] . '</div>';
		}
		*/
		if ( strtolower($returnMode) == 'array' ) {
			return $retAry;
		}
		if ( strtolower($returnMode) == 'string' ) {
			return json_encode( $retAry, JSON_PRETTY_PRINT );
		}
		$tmp = @trim( $GLOBALS['_reqFormat'] );
		header('Content-Type: ' . ( @empty($GLOBALS['_mimeAry'][$tmp]) ? 'text/plain' : $GLOBALS['_mimeAry'][$tmp] ) . ' charset=utf-8');
		print json_encode( $retAry, JSON_PRETTY_PRINT );
		exit;
	}
}
