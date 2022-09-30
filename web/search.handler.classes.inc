<?php

class searchHandler
{
	function getJsonLinkSet( $linkSet = '' )
	{
		@include_once($GLOBALS['_dirAry']['local'].'frame-config.php'); // providing $arrEnginges
		$retAry = array( 'set' => array(), 'html' => '' );
		$tuneAry = array(
			'Journals at Lib4RI' => ''	/* not to show at all */ ,
			'Journals in swisscovery' => 'Journals in swisscovery CH',
		);
		if ( @isset($arrEnginges) && is_array($arrEnginges) ) {
			$searchTerm = @trim(strip_tags(rawurldecode($_GET['find'])));		// OK/standable if empty
			$linkAry = array_filter(array_map('trim',explode(';',$linkSet)));
			foreach($linkAry as $linkPath ) {
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
		$tmp = @trim( $GLOBALS['_reqFormat'] );
		header('Content-Type: ' . ( @empty($GLOBALS['_mimeAry'][$tmp]) ? 'text/plain' : $GLOBALS['_mimeAry'][$tmp] ) . ' charset=utf-8');
		print json_encode( $retAry, JSON_PRETTY_PRINT );
		exit;
	}
}
