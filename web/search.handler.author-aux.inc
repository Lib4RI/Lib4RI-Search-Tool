<?php
	// Scopus will only show 1 author, let check Crossref (and Scopus' Abstact API) for more.
	// If there are, we are going to expand authors for each result in $json4cp array:

	foreach( $json4cp as $resIdx => $resAry ) {

		if ( @sizeof($resAry['author']) !== 1 || @!intval($resAry['DOI']) ) { continue; }

		// CROSSREF APIs:
		// - https://api.crossref.org/works/10.1088/1748-0221/17/07/P07023  VERY SLOW/UNRELIABLE !(?)
		// - https://doi.crossref.org/openurl?pid=my....@uni.org&noredirect=true&format=json&id=doi:10.1088/1748-0221/17/07/P07023
		// - https://doi.crossref.org/openurl?pid=my....@uni.org&noredirect=true&url_ver=Z39.88-2004&rft_id=info:doi/10.1088/1748-0221/17/07/P07023
		$apiAuxUrl = 'https://doi.crossref.org/openurl?pid=' . @strval( W_S_API_CROSSREF_KEY );
		$apiAuxUrl .= '&noredirect=true&format=json&id=doi:' . rawurlencode($resAry['DOI']);

	//	$jsonAuxCode = file_get_contents($apiAuxUrl);	// may cause a time-out!(?)
		$apiAuxTool = new apiQueryUtils('MoreAuthorsByCrossref');
		$jsonAuxCode = $apiAuxTool->makeQuery( $apiAuxUrl );

		if ( !strpos($jsonAuxCode,'}') && stripos($jsonAuxCode,'Resource not found') !== false ) { /* DOI unknown!? Skipping Elsevier + proceeding! */ continue; }

		$autAuxAry = array();
		$jsonAuxAry = json_decode($jsonAuxCode, true);

		if ( @strtolower($jsonAuxAry['status']) == 'ok' && @isset($jsonAuxAry['author']) ) {
			foreach( $jsonAuxAry['author'] as $aAry ) {
				if ( $famName = @trim($aAry['family']) ) {
					$autAuxAry[] = array( 'family' => $famName, 'given' => @trim($aAry['given']) );
				}
			}
		}

		if ( empty($autAuxAry) ) { // Scopus Abstract API - should extend apiQueryScopus !?
			// Scopus Abstract API, example:
			// https://api.elsevier.com/content/abstract/doi/10.1088/1748-0221/17/07/P07023?APIKey=fedc....4321&httpAccept=application/json&field=authors';
			
			$apiAuxUrl = 'https://api.elsevier.com/content/abstract/doi/' . $resAry['DOI']; // do not url-encode!
			$apiAuxUrl .= '?APIKey=' . @strval( W_S_API_SCOPUS_KEY ) . '&httpAccept=application/json&field=authors';

			$apiAuxTool = new apiQueryUtils('MoreAuthorsByScopus');
			$jsonAuxCode = $apiAuxTool->makeQuery( $apiAuxUrl );

			$jsonAuxAry = json_decode($jsonAuxCode, true);
			foreach( $jsonAuxAry['abstracts-retrieval-response']['authors']['author'] as $aAry ) {
				if ( $famName = @trim($aAry['preferred-name']['ce:surname']) ) {
					$autAuxAry[] = array( 'family' => $famName, 'given' => @trim($aAry['preferred-name']['ce:given-name']) );
				}
			}
		}

		if ( sizeof($autAuxAry) > 1 ) {
			$json4cp[$resIdx]['author'] = $autAuxAry;
		}
	}

