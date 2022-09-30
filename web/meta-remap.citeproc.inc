<?php
/*
	Mapping table to get the meta data from different API into a structure as CiteProc can handle it.
	See notes about 'metadata.json' at: https://github.com/seboettg/citeproc-php
*/

$metaMap = array();		// this will be our array/'table' to include for the main configuration/metadata-handling for the APIs.

// ===============================================================================================================
// Example:

$metaMap['citeproc']['testapi'] = array(
	'_numFound'    => 'api_totalResults_key',		/* auxiliary API related value, will ignore '_parent_node' setting resp. you have to re-add it */
	'_parent_node' => 'api_parent_structure',		/* auxiliary API related value, may stay empty but can also be 'results->publications' */
	'_cite_1_by_1' => false,		/* true: CiteProc will render each result individually, a bit slower perhaps, enumations may fail */
	'id'     => 'api_id_field',		/* item_field!? */ 
	'DOI'    => 'api_doi_field',		/* uppercase, for CiteProc! */
	'doi'    => 'api_doi_field',		/* lowercase, should we e.g. grab fieldnames from elsewhere to check this array */
	'title'  => 'api_title_field', 
	'type'   => 'api_type_field',		/* where to get e.g. 'Journal', 'Book' */ 
	'journal' => 'api_journal_field',		/* name of journal' */ 
	'author' => array(
		0 => array(		/* just one index, finally however we are going to use/copy [0] for multiple/further authors */
			'family' => 'api_family_field', 
			'given'  => 'api_given_field', 
			'suffix' => 'api_suffix_field',		/* optional */
		),
	),
	'issued' => array(
		'date-parts' => array(
			0 => 'api_date_year_field',
			1 => 'api_date_month_field',		/*	guess/ok!?	*/
			2 => 'api_date_day_field',		/*	guess/ok!?	*/
		),
	),
	'URL'  => 'url',		/* uppercase, for CiteProc! */
	'url'  => 'url',		/* lowercase, should we e.g. grab fieldnames from elsewhere to check this array */
	'ISSN' => '',			/* uppercase, for CiteProc! */
	'issn' => '',			/* lowercase, should we e.g. grab fieldnames from elsewhere to check this array */
	'page' => '123-145',
	'number-of-pages' => '23',
	'event' => '',
	'abstract' => '',
	'publisher' => '',
	'publisher-place' => '',		/* see Zotero or Endnotes for more meta fields */
);

/*
	API specific helper function (hold by a variable) for CiteProc class to changethe final HTML-presentation of the title.
	see: https://github.com/seboettg/citeproc-php
	
	As implemented right now - keep the naming scheme! For example if the API name is 'abcd' and the name of the metadata
	field is 'xyz' then the variable name for this funcition would be: $websearch_citeproc_polish_abcd_xyz
	With the help for the $metaMap for that API we can check and find such variables/funcations dynamically! :=)
*/
$websearch_citeproc_polish_testapi_doi = function( $cslItem, $renderedText ) {
	if ( $doi = @trim( $cslItem->DOI ) ) {
		return '<a target="_blank" href="https://doi.org/' . $doi . '">' . $renderedText . '</a>';
	}
	return $renderedText;
};


// ===============================================================================================================
// DORA:

$metaMap['citeproc']['dora'] = array(
	'_numFound'    => 'count',		/* auxiliary API related value, will ignore '_parent_node' setting resp. you have to re-add it */
	'_parent_node' => 'items',		/* auxiliary API related value, may stay empty but can also be 'results->publications' */
	'id'    => 'PID',
	'DOI'   => 'doi',
	'doi'   => 'doi',
	'title' => 'title', 
	'type'  => 'genre',
	'author' => array(
		0 => array(
			'family' => 'authors[0]->family_name', 
			'given'  => 'authors[0]->given_name', 
		),
	),
	'issued' => array(
		'date-parts' => array(
			0 => 'year',
		),
	),
	'publisher' => 'publisher',
	'container-title' => 'journal',		/* there is also journal_'abbreviated'
	'volume' => 'volume',
	'issue' => 'issue',
	//		 => 'pg_start',	
	//		 => 'pg_end',	
	'URL'  => 'url',
	'url'  => 'url',
);
/*
$websearch_citeproc_dora_polish_doi = function( $cslItem, $renderedText ) {
	if ( $doi = @trim( $cslItem->DOI ) ) {
		return '<a target="_blank" href="https://doi.org/' . $doi . '">' . html_entity_decode($renderedText) . '</a>';
	}
	return $renderedText;
};
*/
$websearch_citeproc_dora_polish_title = function( $cslItem, $renderedText ) {
	if ( $url = @trim( $cslItem->URL ) ) {
		return '<a target="_blank" href="' . $url . '">' . html_entity_decode($renderedText) . '</a>';
	}
	return $renderedText;
};


// ===============================================================================================================
// Scopus:

$metaMap['citeproc']['scopus'] = array(
	'_numFound'    => 'search-results->opensearch:totalResults',		/* ignoring '_parent_node' ! */
	'_parent_node' => 'search-results->entry',		/* auxiliary API related value, may stay empty but can also be 'results->publications' */
	'id'    => 'dc:identifier',
	'type'  => 'prism:aggregationType',
	'eid'   => 'eid',
	'DOI'   => 'prism:doi',
	'doi'   => 'prism:doi',
	'title' => 'dc:title', 
	'container-title' => 'prism:publicationName',
	'type'  => 'subtypeDescription',
	'author' => array(
		0 => array(
			'family' => 'dc:creator', 
			'given'  => 'dc:creator',
		),
	),
	'issued' => array(
		'date-parts' => array(
			0 => 'prism:coverDate',
		),
	),
	'URL'  => 'link[2]->@href',		/* prism:url for https://api.elsevier.com/content/abstract/scopus_id/85125150300 */
	'url'  => 'link[2]->@href',
//	'issue'  => '',
	'ISSN'   => 'prism:eIssn|prism:issn',
	'ISBN' => 'prism:isbn',
//	'journalAbbreviation' => 'Acta Acust. United Acust.',
//	'language' => 'eng',
//	'note' => 'DOI: 10.1007\/978-3-030-46769-2',
	'page'   => 'prism:pageRange',
	'number-of-pages' => 'prism:pageRange',	
	'volume'   => 'prism:volume',
	'publisher' => 'prism:publisher|prism:publisherName',
	'publisher-place' => 'prism:publisherPlace',
//	'event' => 'Dagstuhl Seminar 15492',
//	'event-place' => 'Schloss Dagstuhl \u2013 Leibniz-Zentrum f\u00fcr Informatik',
//	'collection-title' => 'Springer Water',
//	'collection-number' => '20',
	
);
/*
$websearch_citeproc_scopus_polish_doi = function( $cslItem, $renderedText ) {
	if ( $doi = @trim( $cslItem->DOI ) ) {
		return '<a target="_blank" href="https://doi.org/' . $doi . '">' . html_entity_decode($renderedText) . '</a>';
	}
	return $renderedText;
};
*/
$websearch_citeproc_scopus_prepare_author = function( $nameFull, $argAry ) {
	// to split initials from 'Huber Meier J.F.M.'
	$nameFull = preg_replace('/\.\s+/','.',trim($nameFull));
	if ( $pos = @strrpos($nameFull,' ') ) {
		return ( reset($argAry) == 'given' ? ltrim(substr($nameFull,$pos)) : rtrim(substr($nameFull,0,$pos)) );
	}
	return $nameFull;
};

$websearch_citeproc_scopus_polish_title = function( $cslItem, $renderedText ) {
	if ( $url = @trim( $cslItem->URL ) ) {
		return '<a target="_blank" href="' . $url . '">' . html_entity_decode($renderedText) . '</a>';
	}
	return $renderedText;
};

$websearch_citeproc_scopus_polish_title = function( $cslItem, $renderedText ) {
	if ( $url = @trim( $cslItem->URL ) ) {
		return '<a target="_blank" href="' . $url . '">' . html_entity_decode($renderedText) . '</a>';
	}
	return $renderedText;
};

$websearch_citeproc_scopus_polish_author = function( $cslItem, $renderedText ) {
	$ary = explode(';',$renderedText);
	foreach( $ary as $idx => $aut ) {
		$ary[$idx] = strtok($aut.',',',');
	}
	return implode(';',$ary);
};


// ===============================================================================================================
// SLSP:
// see: https://knowledge.exlibrisgroup.com/Primo/Product_Documentation/Primo/Technical_Guide

$metaMap['citeproc']['slsp'] = array(
	'_numFound'    => 'info->total',		/* ignoring '_parent_node' ! */
	'_parent_node' => 'docs',		/* auxiliary API related value, may stay empty but can also be 'results->publications' */
	'_cite_1_by_1' => true,			/* true: CiteProc will render each result individually, not to make one failed rendering loose all the results */
	'id'    => 'pnx->control->recordid[0]',	/* = alma991114283799705501 */
	'mms'   => 'pnx->display->relation[0]',	/* Oddity: to find number-part of real 'id', see prepare function */
	'DOI'   => 'pnx->display->identifier[0]',
	'doi'   => 'pnx->display->identifier[0]',
	'title' => 'pnx->display->title[0]', 
	'type'  => 'pnx->addata->genre[0]',
	'publisher' => 'pnx->addata->pub[0]|pnx->display->publisher[0]',
	'place' => 'pnx->addata->cop[0]|pnx->display->place[0]',	/* location of publisher */
	'author' => array(
		0 => array(
			'family' => 'pnx->addata->aulast[0]|pnx->display->lds11[0]|pnx->display->creator[0]|pnx->display->contributor[0]',
			'given'  => 'pnx->addata->aufirst[0]|pnx->display->lds11[0]|pnx->display->creator[0]|pnx->display->contributor[0]', 
		),
	),
	'issued' => array(
		'date-parts' => array(
			0 => 'pnx->addata->date[0]|pnx->display->creationdate[0]',
		),
	),
	'URL'  => '@id',
	'url'  => '@id',
	'ISSN'   => 'pnx->addata->issn[0]',
	'issn'   => 'pnx->addata->issn[0]',
	'e-ISSN' => 'pnx->addata->issn[1]|pnx->addata->eissn[0]',	/*	https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=issn,contains,1364-5544&qInclude=facet_rtype%2Cexact%2Cjournals&tab=41SLSP_LIB_DN_CI&search_scope=DN_and_CI&vid=41SLSP_LIB:GO&offset=0&apikey=7777....ffff& */
	'e-issn' => 'pnx->addata->issn[1]|pnx->addata->eissn[0]',	/*	https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=issn,contains,1364-5544&qInclude=facet_rtype%2Cexact%2Cjournals&tab=41SLSP_LIB_DN_CI&search_scope=DN_and_CI&vid=41SLSP_LIB:GO&offset=0&apikey=7777....ffff& */
	'source-id' => 'pnx->display->mms[0]|pnx->control->sourcerecordid[0]',
	'group-id'  => 'pnx->facets->frbrgroupid[0]',
);

$websearch_citeproc_slsp_prepare_mms = function( $apiItem ) {
	// it may be that sometime the 'id' is resp. starts with 9900... (should be 9911...) then there is some chance
	// that the number-part of the the real id can be found after '$$Z' in the realation string!!(?) - Example:
	// https://api-eu.hosted.exlibrisgroup.com/primo/v1/search?q=issn,contains,0168-9002&qInclude=facet_rtype%2Cexact%2Cjournals&tab=41SLSP_LIB_DN_CI&search_scope=DN_and_CI&vid=41SLSP_LIB:GO&offset=0&apikey=7777....ffff&
	$pos = strpos($apiItem,'$$Z');
	if ( $pos === false ) { return ''; }
	return trim( strtok(substr($apiItem,$pos+3).'$','$') );		// should be a number like 9911... or perhaps alma9911...
};

$websearch_citeproc_slsp_prepare_publisher = function( $apiItem ) {
	return ( is_string($apiItem) ? rtrim($apiItem,". \n\r\t\v\x00") : $apiItem );
};

$websearch_citeproc_slsp_prepare_place = function( $apiItem ) {
	return ( is_string($apiItem) ? trim($apiItem,"[]().,:;-, \n\r\t\v\x00") : $apiItem );
};

$websearch_citeproc_slsp_prepare_issued = function( $apiItem ) {
	// remove square-brackets from date:
	return ( is_string($apiItem) ? trim( preg_replace('/\[|\]/','',$apiItem),"-, \n\r\t\v\x00") : $apiItem );
};

$websearch_citeproc_slsp_prepare_author = function( $nameString, $argAry ) {
	// Look+fix proprietary Primo name handling, see https://knowledge.exlibrisgroup.com/Primo/Product_Documentation/Primo/Technical_Guide
	if ( ( $ary = explode('$$Q',$nameString) ) && sizeof($ary) > 1 ) {
		if ( reset($argAry) == 'family' ) {
			return rtrim( strtok($ary[0].',',','),", \n\r\t\v\x00");
		}
		if ( reset($argAry) == 'given' ) {
			return ltrim( strrchr(','.$ary[1],','),", \n\r\t\v\x00");
		}
	}
	return $nameString;
};

$websearch_citeproc_slsp_prepare_title = function( $nameString ) {
	return trim($nameString, ".,;:=- \r\n\t\v\x00");
};

$websearch_citeproc_slsp_polish_title = function( $cslItem, $renderedText ) {
	if ( !( $id = @trim( $cslItem->id ) ) ) {	// e.g.: alma99116721922705522
		return $renderedText;
	}
	if ( !( $vid = @urlencode($GLOBALS['$apiVid']) ) ) {
		$vid = urlencode('41SLSP_LIB:GO');
	}
	return '<a target="_blank" href="https://lib4ri.swisscovery.slsp.ch/discovery/fulldisplay?vid=' . $vid . '&docid=' . $id . '">' . $renderedText . '</a>';
};
 

// ===============================================================================================================
// Web of Science - Widely untested!
// see: https://developer.clarivate.com/apis/woslite/swagger?forUser=c92ed41c1aa6af44610675fb2363299b6a933a9b

$metaMap['citeproc']['wos'] = array(
	'_numFound'    => 'QueryResult->RecordsFound',		/* ignoring '_parent_node' ! */
	'_parent_node' => 'Data',		/* auxiliary API related value, may stay empty but can also be 'results->publications' */
	'id'    => 'UT',
	'DOI'   => 'Other->Identifier.Doi[0]',
	'doi'   => 'Other->Identifier.Doi[0]',
	'ISSN'  => 'Other->Identifier.Issn[0]',
	'issn'  => 'Other->Identifier.Issn[0]',
	'title' => 'Title->Title[0]', 
	'type'  => 'Doctype->Doctype[0]',
	'author' => array(
		0 => array(
			'family' => 'Author->Authors[0]',
			'given'  => 'Author->Authors[0]', 
		),
	),
	'issued' => array(
		'date-parts' => array(
			0 => 'Source->Published.BiblioYear[0]',
		),
	),
	'container-title' => 'Source->SourceTitle[0]',
);
/*
$websearch_citeproc_wos_polish_doi = function( $cslItem, $renderedText ) {
	if ( $doi = @trim( $cslItem->DOI ) ) {
		return '<a target="_blank" href="https://doi.org/' . $doi . '">' . html_entity_decode($renderedText) . '</a>';
	}
	return $renderedText;
};
*/
$websearch_citeproc_wos_prepare_author = function( $nameString, $argAry ) {
	if ( ( $ary = explode(',',$nameString,2) ) && sizeof($ary) > 1 ) {
		if ( reset($argAry) == 'family' ) {
			return rtrim($ary[0]);
		}
		if ( reset($argAry) == 'given' ) {
			return ltrim($ary[1]);
		}
	}
	return $nameString;
};

$websearch_citeproc_wos_polish_title = function( $cslItem, $renderedText ) {
	if ( !( $id = @trim( $cslItem->id ) ) ) {	// e.g.: WOS:000485174000004
		return $renderedText;
	}
	return '<a target="_blank" href="https://www.webofscience.com/wos/woscc/full-record/' . $id . '">' . $renderedText . '</a>';
};

// ===============================================================================================================
// DOAJ / https://doaj.org/api/search/journals/issn:1802-8829
// see: https://www.doaj.org/api/docs#!/CRUD_Journals/get_api_journals_journal_id

$metaMap['citeproc']['doaj'] = array(
	'_numFound'    => 'total',		/* ignoring '_parent_node' ! */
	'_parent_node' => 'results[0]',		/* auxiliary API related value, may stay empty but can also be 'results->publications' */
	'id'    => 'id',
	'ISSN'    => 'bibjson->pissn',
	'issn'    => 'bibjson->pissn',
	'e-ISSN'  => 'bibjson->eissn',
	'e-issn'  => 'bibjson->eissn',
	'since'   => 'bibjson->oa_start', 
	'period'  => 'bibjson->publication_time_weeks',
	'title'   => 'bibjson->title',
	'publisher' => 'bibjson->publisher->name',
	'location' => 'bibjson->publisher->country',
	'URL'     => 'bibjson->apc->url',
	'url'     => 'bibjson->apc->url',
	'license' => 'bibjson->copyright',
);

// ===============================================================================================================
// Sherpa Romeo
// see: https://v2.sherpa.ac.uk/api/object-retrieval-by-id.html

// ===============================================================================================================
// Crossref
// see: https://api.crossref.org/swagger-ui/index.html

// ===============================================================================================================
//
