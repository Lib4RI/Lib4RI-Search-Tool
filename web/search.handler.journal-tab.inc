<?php
	// Generate the tab content for the journal tab (trying to adopt Citeproc handling/look widely)

	//	https://lib4ri.swisscovery.slsp.ch/discovery/search?query=any,contains,Entomology&pfilter=rtype,exact,journals&tab=41SLSP_LIB_DN_CI&search_scope=DN_and_CI&vid=41SLSP_LIB:GO&lang=en&offset=0
	//	https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=title%2Ccontains%2CEntomology&offset=0&limit=10&lang=en&sort=rank&getMore=0&pcAvailability=true&conVoc=true&skipDelivery=true&disableSplitFacets=true&vid=41SLSP_LIB%3AGO&inst=41SLSP_LIB&tab=jsearch_slot&scope=DN_and_CI&apikey=7777....ffff&qInclude=facet_rtype%2Cexact%2Cjournals
	//	https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=title%2Ccontains%2Chydrogen&offset=0&limit=10&lang=en&sort=rank&getMore=0&pcAvailability=true&conVoc=true&skipDelivery=true&disableSplitFacets=true&vid=41SLSP_LIB%3AGO&inst=41SLSP_LIB&tab=jsearch_slot&scope=DN_and_CI&apikey=7777....ffff&qInclude=facet_rtype%2Cexact%2Cjournals

	if ( $twigTemplate = @file_get_contents('./twig/lib4ri-journal-area.twig') ) {	// = HTML code for area for 1 journal
		$htmlTmp = '';
		$jourAry = is_array($json4cp) ? $json4cp : json_decode( ( is_string($json4cp) ? $json4cp : json_encode($json4cp) ), true );
		foreach( $jourAry as $jIdx => $jAry ) {
			// 
			// Add all information that we already have by/from the intial search request of SLSP:
			//
			$dateAry = @array_filter( array(
				$jsonAry['docs'][$jIdx]['pnx']['addata']['date'][0],
				$jsonAry['docs'][$jIdx]['pnx']['addata']['date'][1],
				$jsonAry['docs'][$jIdx]['display']['creationdate'][0],
			)	);
			$relAry = array();
			if ( @isset($jsonAry['docs'][$jIdx]['pnx']['display']['relation']) ) {
				foreach( $jsonAry['docs'][$jIdx]['pnx']['display']['relation'] as $relStr ) {
					$relTmp = ''; // to split+recreate "$$Clater_title$$VAustral Entomology$$QAustral Entomology$$Z99116738095405522"
					if ( !( $pos = strpos($relStr,'$C') ) ) { continue; }
					$relTmp = strtok(substr($relStr,$pos+2).'$','$');
					if ( !stripos($relTmp,'_title') ) { continue; }
					if ( !( $pos = strpos($relStr,'$Q',$pos+2) ) ) { continue; }
					$relTmp = ucwords( strtr($relTmp,'_',' ') );		// to get e.g. "Earlier Title"
					$relTmp .= ( empty($relTmp) ? '' : ': ' ) . rtrim( strtok(substr($relStr,$pos+2).'$','$'), "-., \r\n\t\v\x00" );
					$ary = array_map('trim',explode(':',$relTmp,2));	// expecting e.g. "Earlier Title: Journal of..."
					if ( sizeof($ary) > 1  ) {
						$relAry[] = $ary[0] . ': <a href="./?tab=2&find=' . rawurlencode($ary[1]) . '" title="Search for: ' . $ary[1] . '">' . $ary[1] . '</a>';
					} else {
						$relAry[] = '<a href="./?tab=2&find=' . rawurlencode($relTmp) . '" title="Search for: ' . $relTmp . '">' . $relTmp . '</a>';
					}
				}
			}
			$peerReview = @strtolower($jsonAry['docs'][$jIdx]['pnx']['addata']['peerreview'][0]);
			if ( @in_array($peerReview,array('true','1','yes') ) ) {
				$peerReview = 'Peer Reviewed';
			} elseif ( @in_array($peerReview,array('false','0','no') ) ) {
				$peerReview = 'Not Peer Reviewed';
			} else { // show nothing according to J.F.WS 2022-06-23
				$peerReview = '<!-- Peer Review status unknown -->';
			}
			$almaId = @trim($jsonAry['docs'][$jIdx]['pnx']['control']['recordid'][0]); 	// alma99116818663005522 (or just 99116818663005522 !?)
			$almaId = str_replace( 'alma', '', $almaId ); 	// to ensure the number only
			$issnAry = array_filter( array( @trim($jAry['issn']), @trim($jAry['e-issn']) ) );

			// now preparing+inserting the infos we have into our twig template/code:
			$jIdx = ( $getJsonAPI->searchOffset + $jIdx );
			$jPubler = @trim( $jAry['publisher'] ); 
			$repAry = array(
				'ident'    => $jIdx,
				'available' => 'Availability',
				'created' => '<span style="line-height:2.5ex;">Publication Period: ' . /* $jAry['issued']['date-parts'][0][0] */ @trim($dateAry[0]) . ( @preg_match('/^\d{4}$/',$dateAry[0]) ? '-' : '' ) . '</span>',
				'relation' => implode('<br>',$relAry),
			/*	'publisher' => 'Publisher: <ul class="lib4ri-ul-flat"><li class="lib4ri-li-wide"><span title="' . $jPubler . ( empty($jAry['place']) ? '' : ' ('.$jAry['place'].')' ) . '">' . $jPubler . '</span></li></ul>',	*/
				'publisher' => 'Publisher: <span title="' . $jPubler . ( empty($jAry['place']) ? '' : ' ('.$jAry['place'].')' ) . '">' . $jPubler . '</span>',
				'title'  => trim($jAry['title'], ".,;:=- \r\n\t\v\x00"),
				'issn'   => '<span class="lib4ri-tabul-issn">ISSN</span>: &nbsp;' . ( @empty($jAry['issn']) ? '&nbsp; &mdash;' : $jAry['issn'] ),
				'eissn'  => ( empty($jAry['e-issn']) ? '&nbsp;' : '<span class="lib4ri-tabul-issn">ISSN</span>: &nbsp;' . strval($jAry['e-issn']) ),
			/*	'issn'   => '<!-- prio:' . $issnAuxAry['prio'] . ' -->' . $issnAuxAry['pHTML'],
				'eissn'  => '<!-- prio:' . $issnAuxAry['prio'] . ' -->' . $issnAuxAry['eHTML'],		*/
				'reviewed' => $peerReview,
				'jif'    => 'Journal Impact Factor:',
				'agreement' => '<!-- Agreements -->',
				'embargo-fund' => '<!-- version/embargo/funding -->',
				'license' => '<!-- license -->',
		/*		'empty'  => $auxTag,		*/
				'clickA'  => 'onclick="javascript:lib4riJournalAreaToggle(this.id,\'' . implode(',',$issnAry) . '\',\'' . $almaId . '\');"',
				'clickDiv' => 'onclick="javascript:tog = document.getElementById(\'lib4ri-journal-area-toggle-'.$jIdx.'\'); if(tog && tog.title.indexOf(\'Load\') >= 0) { tog.click(); }"',
			);
			$htmlTmp .= _lib4ri_twigSimple($repAry, $twigTemplate);

		}
		$htmlAry['center'] = $htmlTmp;
	} else {
		$htmlAry['center'] = 'ERROR: Could not open TWIG template!';
	}

