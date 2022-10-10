
// global contants + variables:
if ( lib4riSearchByReload == undefined ) {
	var lib4riSearchByReload = false;
}

if ( lib4riSearchScript == undefined ) {
	var lib4riSearchScript = '/web/search.handler.php';
}

if ( lib4riSearchTab == undefined ) {
	var lib4riSearchTab = [];
	lib4riSearchTab[0] = {  /* unused: */
		label: 'Search the Web!',
		desc : 'Search different online Resources'
	},
	lib4riSearchTab[1] = {
		label: 'Articles, Books, etc.',
		desc : 'Search Scopus, WoS, swisscovery for articles, books, etc.',
	},
	lib4riSearchTab[2] = {
		label: 'Journals',
		desc : 'Search swisscovery for online journals',
	},
	lib4riSearchTab[3] = {
		label: 'Other resources',
		desc : 'Assist searching on other platforms',
	}
}

if ( lib4rSearchAnimData == undefined ) {
	var lib4rSearchAnimData = [
		'https://upload.wikimedia.org/wikipedia/commons/7/7a/',
		'Ajax_loader_metal_512.gif',
		'width:2.5ex; height:2.5ex;'
	];
}

let lib4riUrlParams = new URLSearchParams(window.location.search);
if ( lib4riSearchTabHistory == undefined ) {
	var lib4riSearchTabHistory = [];
	lib4riSearchTabHistory[0] = ( lib4riUrlParams.has('tab') ? Math.max(parseInt(lib4riUrlParams.get('tab')),1) : 1 );
}

if ( lib4riSearchTerm == undefined ) {
	var lib4riSearchTerm = '';
}

// vvvvvvvvvvvv keep this section! vvvvvvvvvv
//		[config-by-php]
// ^^^^^^^^^^^^ keep this section! ^^^^^^^^^^

// global functions:

if ( typeof(lib4riSearchEncodeURI) != 'function' ) {
	function lib4riSearchEncodeURI( str = '' ) {
		return encodeURI(str).replace(/\+/g,'%2B').replace(/\&/g,'%26');
	}
}

if ( typeof(lib4riSearchDecodeURI) != 'function' ) {
	function lib4riSearchDecodeURI( str = '' ) {
		return decodeURI( str.replace(/\%26/g,'&').replace(/\%2B/g,'+') );
	}
}

if ( typeof(lib4riJsonFetchOld) != 'function' ) {
	function lib4riJsonFetchOld(urlFile, callback) {
		let httpReq = new XMLHttpRequest();
		httpReq.overrideMimeType("application/json");
		httpReq.open("GET", urlFile, true);
		httpReq.onreadystatechange = function() {
			if (httpReq.readyState === 4 && httpReq.status == "200") {
				callback(httpReq.responseText);
			}
		}
		httpReq.send(null);
	}
}

if ( typeof(lib4riJsonFetch) != 'function' ) {
	function lib4riJsonFetch(urlFile, callback) {
		let httpReq = new XMLHttpRequest();
		httpReq.ontimeout = function () {
			console.error('Timeout for Request ' + urlFile);
		};
		httpReq.onload = function() {
			if (httpReq.readyState === 4) {
				if ( httpReq.status === 0 || ( httpReq.status >= 200 && httpReq.status < 400) ) {
					callback(httpReq.responseText);
				} else {
					console.error('Request failed (status: ' + httpReq.status + ') for: ' + urlFile);
				}
			}
		};
		httpReq.overrideMimeType("application/json");
		httpReq.timeout = 60000;
		httpReq.open("GET", urlFile, true);
		httpReq.send(null);
	}
}

if ( typeof(lib4riSearchIsTermEmpty) != 'function' ) {
	function lib4riSearchIsTermEmpty(term = '') {
		let str1 = term.trim().substring(0,10);
		if ( str1 == '' ) {
			return true;
		}
		if ( str1.length >= 10 ) { // also consider the tab-descriptions as empty:
			for(i=1;i<lib4riSearchTab.length;i++) {
				let desc = lib4riSearchTab[i]['desc'].toString();
				if ( desc.substring(0,10) == str1 ) {
					return true;
				}
			}
		}
		return false;
	}
}

if ( typeof(lib4riSearchFormTabChoice) != 'function' ) {
	function lib4riSearchFormTabChoice(tab = 0) {
		let bbElem = document.getElementById('lib4ri-search-form-tab');
		if ( bbElem ) {
			if ( tab === 0 ) {
				tab = lib4riSearchTabHistory[0];
			}
			bbElem.value = tab;

			bbElem = document.getElementById('lib4ri-search-form-term');
			if ( bbElem && lib4riSearchIsTermEmpty(bbElem.value) ) {
				for(i=1;i<lib4riSearchTab.length;i++) {
					if ( tab <= i && lib4riSearchTab[i]['desc'] != undefined ) {
						bbElem.style.color = '#899';
						bbElem.value = lib4riSearchTab[i]['desc'];
						break;
					}
				}
			}
		}
	}
}

if ( typeof(lib4riSearchFormTabInput) != 'function' ) {
	function lib4riSearchFormTabInput(term = '') {
		let bbElem = document.getElementById('lib4ri-search-form-term');
		if ( bbElem && lib4riSearchIsTermEmpty( ( term == '' ? bbElem.value : term ) ) ) {
			if ( bbElem.value != '' ) { bbElem.value = ''; }
			bbElem.style.color = '#100';
		}
	}
}

if ( typeof(lib4riSearchDelDivClass) != 'function' ) {
	function lib4riSearchDelDivClass(bbHtml, bbClass = 'lib4ri-search-anim') {
		// auxiliary, indended to remove <div class="lib4ri-search-anim...">anything-expect-of-div</div> from HTML code - not happy about this though...
		let tmpA = '';
		let tmpZ = '';
		let pos = ( bbClass != '' ? bbHtml.indexOf('class="'+bbClass) : -1 );
		while( pos >= 0 ) {
			tmpA = bbHtml.substring(0,pos);
			tmpA = tmpA.substring(0,tmpA.lastIndexOf('<div'));
			tmpZ = bbHtml.substring(pos);
			tmpZ = tmpZ.substring(tmpZ.indexOf('</div>')+6);
			bbHtml = tmpA + tmpZ;
			pos = bbHtml.indexOf('class="'+bbClass);
		}
		return bbHtml;
	}
}

if ( typeof(lib4riSearchAnimLoad) != 'function' ) {
	function lib4riSearchAnimLoad(bbName,bbCount = 0) {
		let bbClass = ( ( bbName.indexOf('-bentobox-') > 0 ) ? 'lib4ri-search-anim-block' : 'lib4ri-search-anim-inline' );
		let bbAnim = '<img src="' + lib4rSearchAnimData[0] + lib4rSearchAnimData[1] + '" style="' + lib4rSearchAnimData[2] + '">'; // ...or as CSS!?
		let bbElem = document.getElementById(bbName);
		if ( bbElem && bbAnim && lib4rSearchAnimData[1] ) {
	//		bbElem.innerHTML = ( ( bbClass == 'lib4ri-search-anim-block' ) ? '' : bbElem.innerHTML + ' ' ) + '<div class="' + bbClass + '">' + bbAnim + '</div>';
			bbElem.innerHTML = lib4riSearchDelDivClass(bbElem.innerHTML) + '<div class="' + bbClass + '">' + bbAnim + '</div>';
		}

		return;	// for the time being, code below may overwrite content possibly added in meantime!

		setTimeout( function() {
				let bbElem = document.getElementById(bbName);
				if ( bbElem && lib4rSearchAnimData[1] && bbElem.innerHTML.indexOf(lib4rSearchAnimData[1]) > 0 ) {
					bbElem.innerHTML = '<div class="' + bbClass + '">';
					bbElem.innerHTML += bbAnim + ' <i>...seems to take some longer...<i>' + '</div>';
				}
			}, 5000 + (bbCount * 150)
		);
		setTimeout( function() {
				let bbElem = document.getElementById(bbName);
				if ( bbElem && lib4rSearchAnimData[1] && bbElem.innerHTML.indexOf(lib4rSearchAnimData[1]) > 0 ) {
					bbElem.innerHTML = '<div class="' + bbClass + '">';
					bbElem.innerHTML += bbAnim + ' <i>...seems to take some longer... - much longer...<i>' + '</div>';
				}
			}, 11000 + (bbCount * 150)
		);
		setTimeout( function() {
				let bbElem = document.getElementById(bbName);
				if ( bbElem && lib4rSearchAnimData[1] && bbElem.innerHTML.indexOf(lib4rSearchAnimData[1]) > 0 ) {
					bbElem.innerHTML = '<div class="' + bbClass + '">';
					bbElem.innerHTML += bbAnim + ' <i>...seems to take some longer... - much longer...<i>';
		/*	*/		bbElem.innerHTML += '&nbsp; <span style="white-space:nowrap;">...<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/5/5b/Tox_poisonous.svg/300px-Tox_poisonous.svg.png" style="width:2.8ex; height:2.5ex; position:relative; top:0.7ex; margin:0 -.1ex 0 -.05ex;"><!-- source: https://commons.wikimedia.org/wiki/File:Tox_poisonous.svg -->...</span>';
					bbElem.innerHTML += '</div>';
				}
			}, 18000 + (bbCount * 150)
		);
		setTimeout( function() {
				let bbElem = document.getElementById(bbName);
				if ( bbElem && lib4rSearchAnimData[1] && bbElem.innerHTML.indexOf(lib4rSearchAnimData[1]) > 0 ) {
					bbElem.innerHTML = '<!-- Nothing Got :=( -->';
				}
			}, 26000 + (bbCount * 150)
		);
	}
}

if ( typeof(lib4riSearchPerform) != 'function' ) {
	function lib4riSearchPerform(bbName, apiArgStr, bbCount = 0, searchOffset = -1, searchLimit = -1) {
		if ( lib4riSearchScript == undefined || lib4riSearchScript == '' || apiArgStr == '' ) {
			console.log('Error: lib4riSearchScript is not defined!');
			return;
		}
		if ( lib4rSearchAnimData == undefined || lib4rSearchAnimData[1] == undefined ) {
			console.log('Error: No Bentobox Animation set!');
			return;
		}

		setTimeout(
			lib4riJsonFetch( lib4riSearchScript+apiArgStr, function(text){
				let jsonData = JSON.parse(text);
				if ( jsonData && ( jsonData['html'] != undefined || jsonData['error'] != undefined ) ) {
					if ( jsonData['error'] != undefined ) {
						console.log( jsonData['error'] );
					}
					let htmlData = [];
					if ( typeof(jsonData['html']) == 'array' || typeof(jsonData['html']) == 'object' ) {
						htmlData = jsonData['html'];
					} else {
						htmlData[bbName] = jsonData['html'];
					}

					for( bbName in htmlData ) {
						let bbElem = document.getElementById(bbName);
						if ( bbElem ) {
							bbElem.innerHTML = lib4riSearchDelDivClass(bbElem.innerHTML);	// remove animations
							let htmlGot = ( ( htmlData[bbName] == undefined ) ? '' : decodeURIComponent(htmlData[bbName]) );
							if ( searchOffset >= 0 ) {
								// no prio-handling, just/simply add new HTML content onto the existing one
								bbElem.innerHTML += htmlGot;
								continue;
							}
							let prioNow = ( ( bbElem.innerHTML.length > 10 && bbElem.innerHTML.indexOf('<!-- prio:') === 0 ) ? parseInt( bbElem.innerHTML.substring(10,12) ) : -1 );
							let prioGot = -1;
							let prioAct = '';
							if ( htmlGot.indexOf('<!-- prio:') === 0 ) {
								prioAct = htmlGot.substring(10,12).trim();		// can be a number 0-9 (+space) or 'cut-from-char' (+space)
								prioGot = parseInt( prioAct );
							}
							if ( prioAct == '^' ) { // adding new/got HTML  at the beginning of inner HTML:
								htmlGot = ( jsonData['error'] == undefined ? '' : jsonData['error'] + '<br>' ) + htmlGot.substring( htmlGot.indexOf('>') + 1 );
								if ( prioNow >= 0 ) {
									let pos = bbElem.innerHTML.indexOf('>') + 1;	// end/after prio tag
									bbElem.innerHTML = bbElem.innerHTML.substring(0,pos) + htmGot + bbElem.innerHTML.substring(pos);
								} else {
									bbElem.innerHTML = '<!-- prio:0 -->' + htmGot + bbElem.innerHTML;
								}
							}
							else if ( prioAct == '~' ) { // adding new/got HTML at the end of inner HTML:
								htmlGot = htmlGot.substring( htmlGot.indexOf('>') + 1 );
								bbElem.innerHTML += ( jsonData['error'] == undefined ? '' : '<br>' + jsonData['error'] + '<br>' ) + htmlGot;
							}
							else if ( prioAct != '' && prioGot < 1 && prioAct !== '0' ) { // try to replace only the part after 'prioAct' character
								let pos = bbElem.innerHTML.indexOf(prioAct) + 1;	// end/after prio tag
								htmlGot = htmlGot.substring( htmlGot.indexOf('>') + 1 );
								htmlGot = ( jsonData['error'] == undefined ? '' : ( pos > 1 ? '<br>' : '' ) + jsonData['error'] + '<br>' ) + htmlGot;
								bbElem.innerHTML = ( ( pos > 1 ) ? bbElem.innerHTML.substring(0,pos) : bbElem.innerHTML ) + htmlGot;
							}
							else if ( prioNow < 0 ) { // complete replacement of inner HTML:
								bbElem.innerHTML = '<!-- prio:0 -->' + ( jsonData['error'] == undefined ? '' : jsonData['error'] + '<br>' ) + htmlGot;
							}
							else if ( prioAct === '0' || prioGot > 0 ) {
								if ( prioGot > prioNow ) { // complete replacement of inner HTML:
									bbElem.innerHTML = ( jsonData['error'] == undefined ? '' : jsonData['error'] + '<br>' ) + htmlGot;
								}
							}

							if ( bbElem.style.display == 'none' && bbElem.innerText.trim() != '' ) {
								bbElem.style.display = 'auto';
							}
							// check, if not readable content then hide the div entirely:
							for( let bbChild of bbElem.parentNode.children ) {
								if ( bbChild.innerText.trim() == '' ) {
									bbChild.style.display = 'none';
								} else if ( bbChild.style.display == 'none' ) {
									bbChild.style.display = 'revert';
								}
							}
						}
					}


					// for journal only: We must find out how many Journal we found in order to update the avaiability hint right afterwards.
					// ...I am not so happy about having this here - to be tuned!
					if ( jsonData['total-found'] != undefined && jsonData['total-found'] > 0 ) {
						if ( lib4riSearchTabHistory[0] == 2 && apiArgStr.concat('&').indexOf('api=journal&') > 0 ) {
							let bbElemAry = document.getElementsByClassName('lib4ri-journal-area-toggle-link');
							for ( b=Math.max(searchOffset,0);b<bbElemAry.length;b++ ) {
								let idToggle = bbElemAry[b].id;		// for example: lib4ri-journal-area-toggle-12345
								let dataTmp = bbElemAry[b].getAttribute('onclick');	// = "javascript:lib4riJournalAreaToggle(this.id,'2673-4141','99116818663005522');"
								if ( idToggle && dataTmp && dataTmp.indexOf(',') ) {
									let idBase = idToggle.substring(0,idToggle.indexOf('-area-toggle')); // = lib4ri-journal
									let idArea = idToggle.substring(idToggle.lastIndexOf('-')+1);		// = 12345
									let dataAry = dataTmp.split("'");
									setTimeout( function() {
										lib4riSearchJournalDetail( dataAry[1], dataAry[3], 'swisscovery', 'available,linklist', idBase, idArea );
									}, (300 * (b + Math.max(searchOffset,0) ) + 150) );
								}
								if ( searchOffset < 1 && bbElemAry.length == 1 ) {
									// Unfold automatically with only one journal area, in conjunction with 'Unfold All'
									setTimeout( function() {
										let bbElem = document.getElementById(idToggle);
										if ( bbElem ) { bbElem.click(); }
									}, (300 * (b + Math.max(searchOffset,0) ) + 600) );
								}
							}
						}
					}


				}
			}),
			(bbCount * 150 + 200)
		);

		lib4riSearchAnimLoad(bbName,bbCount);
	}
}

if ( typeof(lib4riSearchLinkDirect) != 'function' ) {
	function lib4riSearchLinkDirect(searchFind = '',urlUpdate = false) {
		let bbElem = document.getElementById('lib4ri-search-form-term');
		if ( !( bbElem && bbElem.value && !lib4riSearchIsTermEmpty(bbElem.value) ) ) {
			return; // do not react on empty/false inputs!
		}

		searchFind = decodeURI( searchFind.trim() );

		if ( !urlUpdate ) {
			let pos = (window.location.href+'?').indexOf('?');
			let searchLink = window.location.href.substr(0,pos) + '?';

			if ( lib4riSearchTabHistory != undefined ) {
				searchLink += 'tab=' + lib4riSearchTabHistory[0] + '&';
			}
			if ( searchFind != '' ) {
				searchLink += 'find=' + encodeURI( searchFind );
			}
			return searchLink;
		}

		if ( bbElem = document.getElementById('lib4ri-search-form-hint') ) {
			if ( bbElem.innerHTML.indexOf('<form') >= 0 ) {
				bbElem.innerHTML = '';		// clear the initial tab-selection form
			}
		}

		let url = new URL(window.location);
		urlUpdate = false;
		if ( lib4riSearchTabHistory != undefined ) {
			if ( url.searchParams.has('tab') && parseInt(url.searchParams.get('tab')) != lib4riSearchTabHistory[0] ) {
				url.searchParams.set('tab', lib4riSearchTabHistory[0] );
				urlUpdate = true;
			}
		}
		if ( searchFind != '' ) {
			if ( url.searchParams.has('find') && decodeURI( url.searchParams.get('find') ) != searchFind ) {
				url.searchParams.set('find', searchFind );		// seems to encode!?
				urlUpdate = true;
			}
		}
		if ( urlUpdate ) {
			window.history.pushState( {}, '', url );
		}
	}
}

if ( typeof(lib4riSearchBentoboxTuner) != 'function' ) {
	function lib4riSearchBentoboxTuner(searchFind = '',searchOffset = -1, searchLimit = -1) {
		if ( searchFind && searchFind != '' ) {
			let bbElem = document.getElementById('lib4ri-back-container');
			if ( bbElem && bbElem.style && bbElem.style.display == 'none' ) { bbElem.style = 'display:auto'; }

			let bbElemAry = document.getElementsByClassName('lib4ri-bentobox-result');
			for(b=0;b<bbElemAry.length;b++) {

				let idTabNow = bbElemAry[b].parentNode.parentNode.parentNode.id; // e.g. lib4ri-result-container-2
				if ( parseInt(idTabNow.slice(-1)) != lib4riSearchTabHistory[0] ) {
					continue;	// we are on the wrong tab!
				}
				
				let bbAry = bbElemAry[b].id.trim().toLowerCase().split('-'); // e.g. lib4ri-bentobox-dora-psi
				let argStr = '?req=json&api=' + bbAry[2];
				if ( bbAry[3] != undefined && bbAry[3] != '' ) {
					argStr += '&scope=' + bbAry[3];
				}
				if ( bbElemAry[b].title != undefined && bbElemAry[b].title != '' ) {
					argStr += '&' + bbElemAry[b].title.trim();		// config tweak!?
				}
				if ( searchOffset >= 0 || searchLimit >= 0 ) {
					if ( searchOffset >= 0 ) {
						argStr += '&offset=' + searchOffset;
					}
					if ( searchLimit >= 0 ) {
						argStr += '&limit=' + searchLimit;
					}
				}
				else if ( lib4riUrlParams ) {
					// we are not going to use location.search directly/entirely, and only forward query parameters 
					// we know/want form our global lib4riUrlParams array (which holds all location.search values):
					if ( searchOffset < 0 && lib4riUrlParams.has('offset') ) {
						let searchTmp = lib4riUrlParams.get('offset').trim();
						if ( searchTmp != '' ) { argStr += '&offset=' + parseInt(searchTmp); }
					}
					if ( searchLimit < 0 && lib4riUrlParams.has('limit') ) {
						let searchTmp = lib4riUrlParams.get('limit').trim();
						if ( searchTmp != '' ) { argStr += '&limit=' + parseInt(searchTmp); }
					}
				}
				argStr += '&find=' + escape(encodeURI(searchFind));	// not required here but intentionally at the end
				lib4riSearchPerform(bbElemAry[b].id, argStr, b, searchOffset, searchLimit);
			}
		}
	}
}

if ( typeof(lib4riSearchBentoboxNext) != 'function' ) {
	function lib4riSearchBentoboxNext(searchFind = '',searchOffset = -1, elemId = '', elemDesc = '') {
		if ( elemId != '' ) {
			let bbElem = document.getElementById( elemId.replace('-link-','-desc-') );
			if ( bbElem ) {
				bbElem.innerHTML = elemDesc;
			}
		}
		lib4riSearchBentoboxTuner(searchFind,searchOffset);
	}
}

if ( typeof(lib4riSearchFormSubmitByLink) != 'function' ) {
	function lib4riSearchFormSubmitByLink() {
		let bbElem = document.getElementById('lib4ri-search-form-term');
		if ( bbElem && bbElem.value && !lib4riSearchIsTermEmpty(bbElem.value) ) {
			bbElem = document.getElementById('lib4ri-search-form');
			if ( bbElem ) {
				bbElem.submit();
				return true;
			}
		}
		return false;
	}
}

if ( typeof(lib4riSearchFormSubmit) != 'function' ) {
	function lib4riSearchFormSubmit(event) {
		if ( !lib4riSearchByReload ) {
			event.preventDefault();		// we are going to submit() ourselves!
		}
		let bbElem = document.getElementById('lib4ri-search-form-term');
		if ( bbElem && bbElem.value && !lib4riSearchIsTermEmpty(bbElem.value) ) {
			lib4riSearchTerm = bbElem.value.trim();		// global update!
			if ( 6 < 7 || !lib4riSearchByReload ) {		// due to tabs show it currently nonetheless
				lib4riSearchLinkDirect(lib4riSearchTerm,true);
			}
			if ( !lib4riSearchByReload ) {
				lib4riSearchBentoboxTuner(lib4riSearchTerm);
			}
		}
		else if ( lib4riSearchByReload ) {		// we got a empty/false input, so do not react!
			event.preventDefault();		// we are going to submit() ourselves!
		}
	}
}

if ( typeof(lib4riSearchTabToggle) != 'function' ) {
	function lib4riSearchTabToggle(tabWanted = 0) {
		if ( tabWanted == 0 ) {
			tabWanted = lib4riSearchTabHistory[0];
		}
		else if ( lib4riSearchTabHistory[0] != tabWanted ) {
			let bbElem = document.getElementById('lib4ri-search-form-tab');
			if ( bbElem ) {
				bbElem.value = tabWanted;	// for lib4riSearchByReload: remember the current tab when/at reload.
			}
			lib4riSearchTabHistory[0] = tabWanted;
		}
		else {
			return; // already on the intended tab
		}
		let bbElemAry = document.getElementsByClassName('lib4ri-search-tab');
		for(b=0;b<bbElemAry.length;b++) {
			let idTabNow = parseInt(bbElemAry[b].id.slice(-1));	// we need the ending number e.g. in: lib4ri-search-tab-1
			if ( idTabNow == tabWanted ) {
				bbElemAry[b].classList.remove('lib4ri-search-tab-off');
				bbElemAry[b].classList.add('lib4ri-search-tab-on');
			} else {
				bbElemAry[b].classList.remove('lib4ri-search-tab-on');
				bbElemAry[b].classList.add('lib4ri-search-tab-off');
			}
			let bbElem = document.getElementById('lib4ri-result-container' + '-' + idTabNow);
			if ( bbElem ) { // toggle the corresponding result container as well:
				bbElem.style = ( ( idTabNow == tabWanted ) ? 'display:auto' : 'display:none' );
			}
		}
		let bbElem = document.getElementById('lib4ri-search-form-term');
		if ( bbElem && bbElem.value && !lib4riSearchIsTermEmpty(bbElem.value) ) {
			lib4riSearchTerm = bbElem.value.trim(); // global update!
			if ( lib4riSearchTerm && lib4riSearchTerm != null ) {

				lib4riSearchLinkDirect(lib4riSearchTerm,true);
				if ( lib4riSearchTabHistory[tabWanted] == undefined || lib4riSearchTabHistory[tabWanted] != lib4riSearchTerm ) {
					lib4riSearchTabHistory[tabWanted] = lib4riSearchTerm;
					setTimeout( function() {
							lib4riSearchBentoboxTuner(lib4riSearchTerm);
						},
						500
					);
				}

			}
		}
		// see if there is a label to update
		bbElemAry = document.getElementsByClassName('lib4ri-bentobox-label');
		if ( bbElemAry && bbElemAry[0] != undefined && lib4riSearchTerm && lib4riSearchTerm != null ) {
			for(b=0;b<bbElemAry.length;b++) {
				let idTabNow = bbElemAry[b].parentNode.parentNode.parentNode.id; // e.g. lib4ri-result-container-2
				if ( parseInt(idTabNow.slice(-1)) == lib4riSearchTabHistory[0] ) {
			/*
					let bbPos = bbElemAry[b].innerHTML.indexOf('<!--');
					if ( bbPos > 0 ) {
						let bbTmp = bbElemAry[b].innerHTML.substring(bbPos+4).trim();
						bbTmp = bbTmp.substring(0,bbTmp.indexOf('-->')).trim();
						bbElemAry[b].innerHTML = bbElemAry[b].innerHTML.substring(0,bbPos).trim() + ' <!-- ' + bbTmp + ' -->' + bbTmp;
						bbElemAry[b].innerHTML += ' <span class="lib4ri-bentobox-searchterm">' + lib4riSearchTerm + '</span>';
					}
			*/
					if ( bbElemAry[b].id == 'lib4ri-bentobox-label-2-1-1' || bbElemAry[b].title == 'Journal List' ) {
						let idToggle = 'journal-unfold-all';
						bbElemAry[b].id = idToggle;
						setTimeout( function() {
							let bbElem = document.getElementById(idToggle);
							// Only show the toggle link if there will be more than 1 result (since with we auto-unfold a sole one):
							if ( bbElem && document.getElementsByClassName('lib4ri-journal-area').length != 1 ) { 
								bbElem.innerHTML = '<div style="display:inline-block; width:50%; text-align:left; font-weight:700;">Journal List</div>';
								bbElem.innerHTML += '<div style="display:inline-block; width:47%; text-align:right;"><a href="jav'+'asc'+'ript:" onclick="jav'+'asc'+'ript:lib4riJournalAreaShowAll()" style="font-style:italic; font-weight:400;">unfold all</a></div>';
							}
						}, 3000	);
					}
			//		else if ( parseInt(idTabNow.slice(-1)) != 2 ) {	// not on journal tab
			//			bbElemAry[b].innerHTML = '&nbsp;';
			//		}
				}
			}
		}

		setTimeout( "lib4riSearchLinkUpdate()", 300 );		// give it a heart-beat to let it toggle completely
	}
}

// Compose the search from, also adding event handler onto it:
if ( typeof(lib4riSearchFormAdd) != 'function' ) {
	function lib4riSearchFormAdd() {
		if ( lib4riSearchForm != undefined ) {
			return;		// search form (variable) already set elseway
		}
		var lib4riSearchForm = document.getElementById('lib4ri-search-form');
		if ( lib4riSearchForm ) {
			return;		// search form is existing with expected name
		}

		let searchFind = ( lib4riUrlParams.has('find') ? decodeURI(lib4riUrlParams.get('find')).replace(/(<([^>]+)>)|\?|,|\!/gi,'').replace(/\:\s*|\'|\+/g,' ').trim() : '' );

		let searchLimit = ( lib4riUrlParams.has('limit') && lib4riUrlParams.get('limit').trim() != '' ? parseInt(lib4riUrlParams.get('limit')) : '' );
		let searchOffset = ( lib4riUrlParams.has('offset') && lib4riUrlParams.get('offset').trim() != '' ? parseInt(lib4riUrlParams.get('offset')) : '' );

		let lsfArea = document.getElementById('lib4ri-search-form-area');
		if ( lsfArea && lsfArea.innerHTML.indexOf('<form') < 0 ) {
			let submitButton = '';
			if ( !lib4riSearchByReload ) {
				submitButton = '<!-- will transfer X + Y --><input type="image" id="lib4ri-search-form-submit" alt="=!=" class="lib4ri-search-icon-button" />';
			} else {
				submitButton = '<a href="javas'+'cript:" onclick="javas'+'cript:lib4riSearchFormSubmitByLink();" id="lib4ri-search-form-icon" />';
			}
			lsfArea.innerHTML = '<form id="lib4ri-search-form" class="lib4ri-search-form" style="white-space:nowrap">'
				+ '<input id="lib4ri-search-form-tab" type="hidden" name="tab" value="' + lib4riSearchTabHistory[0] + '">'
				+ '<input id="lib4ri-search-form-limit" type="hidden" name="limit" value="' + searchLimit + '">'
				+ '<input id="lib4ri-search-form-offset" type="hidden" name="offset" value="' + searchOffset + '">'
				+ '<input id="lib4ri-search-form-term" type="text" name="find" value="' /* + searchFind, but quotes may get lost!? */ + '" '
				+ 	'onclick="lib4riSearchFormTabInput(this.value);" onFocus="lib4riSearchFormTabInput(this.value);">'
				+ ( submitButton ? submitButton : '<button id="lib4ri-search-form-submit" type="submit">Search!</button>' )  + '</form>';
			// instantly try now to load the form added above:
			if ( lib4riSearchForm = document.getElementById('lib4ri-search-form') ) {
				let searchTmp = document.getElementById('lib4ri-search-form-term');
				if ( searchTmp ) {
					searchTmp.value = searchFind;	// this should maintain quoted terms, also safer a bit
				}
				lib4riSearchForm.removeEventListener('submit', lib4riSearchFormSubmit);	// safety clean-up
				lib4riSearchForm.addEventListener('submit', lib4riSearchFormSubmit);
			}

			// let the user decide which (tab to) search:
			lsfArea = document.getElementById('lib4ri-search-form-hint');
			if ( lsfArea ) {
				let htmlTmp = '';
				for(i=1;i<lib4riSearchTab.length;i++) {
					htmlTmp += '<div><input type="radio" class="lib4ri-search-form-radio" name="tab" value="' + i + '" ' 
						+ ( Math.max(parseInt(lib4riSearchTabHistory[0]),1) == i ? 'checked' : '' )
						+ ' onclick="lib4riSearchFormTabChoice(this.value);"> ' + lib4riSearchTab[i]['label'] + ' &nbsp; </div>';
				}
				lsfArea.innerHTML = '<form id="lib4ri-search-form-tab-choice" action=""><div>Search for: &nbsp;</div>' + htmlTmp + '</form>';
			}
		}

		if ( searchFind != null && searchFind != '' ) {
			setTimeout( function() { lib4riSearchLinkDirect(searchFind,true); }, 750 );
		}
	}
}

if ( typeof(lib4riSearchLinkUpdate) != 'function' ) {
	function lib4riSearchLinkUpdate(searchFind = '') {
		if ( searchFind.trim() != '' ) {
			searchFind = decodeURI(searchFind).trim();
		} else if ( lib4riSearchTerm != '' ) {
			searchFind = lib4riSearchTerm;
		}

		if ( lib4riSearchLinkValue == undefined ) {
			var lib4riSearchLinkValue = searchFind;		// initiate for later comparison!
		} else if ( lib4riSearchLinkValue == searchFind ) {
			return;			// nothing to do
		} else {
			lib4riSearchLinkValue = searchFind;		// update global/initated var
		}

		let bbElemAry = document.getElementsByClassName('lib4ri-bentobox-linkset');
		if ( !( bbElemAry && bbElemAry[0] != undefined ) ) {
			return;		// no linkset areas at all !(?)
		}
		if ( lib4riSearchScript == undefined || lib4riSearchScript == '' ) {
			console.log('Error: lib4riSearchScript is not defined!');
			return;
		}

		for(b=0;b<bbElemAry.length;b++) {
			let idAry = bbElemAry[b].id.split('-');		// to split lib4ri-bentobox-linkset-1-3-2
			let idTabNow = parseInt(idAry[3]);
			if ( idTabNow == 0 ) {
				idTabNow = parseInt(bbElemAry[b].parentNode.parentNode.parentNode.id.slice(-1)); // e.g. 2 from lib4ri-result-container-2
			}
			if ( idTabNow == lib4riSearchTabHistory[0] ) {
				let bbName = bbElemAry[b].id;
				let argStr = '?linkset=' + lib4riSearchEncodeURI(bbElemAry[b].title) + '&find=' + escape(encodeURI(searchFind));
				setTimeout(
					lib4riJsonFetch( lib4riSearchScript+argStr, function(text){
						let jsonData = JSON.parse(text);
						let bbElem = document.getElementById(bbName);
						if ( !( bbElem ) ) {
							console.log( 'ERROR: HTML element with id \'' + bbName + '\' could not be found!' );
						}
						else if ( jsonData && ( jsonData['html'] != undefined || jsonData['error'] != undefined ) ) {
							if ( jsonData['error'] != undefined ) {
								console.log( jsonData['error'] );
							}
							if ( jsonData['set'] != undefined && ( typeof(jsonData['set']) == 'array' || typeof(jsonData['set']) == 'object' ) ) {
								bbElem.innerHTML = ( jsonData['error'] == undefined ? '' : jsonData['error'] + '<br>' );
								for(i=0;i<jsonData['set'].length;i++) {
									bbElem.innerHTML += ( i > 0 ? "\r\n" : '' ) + '<div class="lib4ri-bentobox-linkset-link">' + jsonData['set'][i] + '</div>';
								}
							} else { // old/former approach with full/final HTML:
								bbElem.innerHTML = ( jsonData['error'] == undefined ? '' : jsonData['error'] + '<br>' );
								bbElem.innerHTML += ( jsonData['html'] == undefined ? '<!-- no HTML for '+bbName+' -->' : jsonData['html'] );
							}
						}
					}),
					(100 * b + 50)
				);
			}
		}
	}
}


if ( typeof(lib4riSearchJournalDetail) != 'function' ) {
	function lib4riSearchJournalDetail(issnList = '',almaId = '',apiName = '',detailList = '',idBase = '', idArea = '') {
			if ( detailList != '' ) {
			let htmlTarget = idBase + ( detailList.split(',').length == 1 ? '-'+detailList+'-' : '-@-' ) + idArea;
			let argStr = '?req=json'
			argStr += '&api=' + 'JournalDetail';
			argStr += '&remote=' + apiName;
			argStr += '&issn=' + issnList;
			argStr += '&alma=' + almaId;
			argStr += '&detail=' + detailList;
			argStr += '&target=' + htmlTarget;
			argStr += '&find=' + ( lib4riSearchTerm ? lib4riSearchTerm : '*' );
			// insert in to a HTML element (id will be'lib4ri-journal-'+detAry[0]+'-'+idArea ) the data returned
			// from the API request (= argStr), whereof int(idArea) just influences the individual delay for this:
			lib4riSearchPerform(htmlTarget, argStr, parseInt(idArea) );
		}
	}
}

if ( typeof(lib4riJournalAreaToggle) != 'function' ) {
	function lib4riJournalAreaToggle(idToggle = '',issnList = '',almaId = '') { // idToggle: lib4ri-journal-area-toggle-12345
		let jToggle = document.getElementById(idToggle);
		let jArea = document.getElementById(idToggle.replace('-toggle-','-info-'));		// to get: lib4ri-journal-area-info-12345

		if ( jToggle.title.indexOf('Hide') >= 0 ) {
			jArea.style = "display:none;";
			jToggle.title = 'Show details';
			jToggle.innerHTML = '<span class="lib4ri-toggle-closed" />';
			return;
		}

		let initLoad = ( jToggle.title.toLowerCase().indexOf('load') >= 0 ); // not to load it once again - 'load' we do at 1st time, then 'hide', then 'show'

		jArea.style = "display:auto;";
		jToggle.title = 'Hide details';
		jToggle.innerHTML = '<span class="lib4ri-toggle-opened" />';

		if ( !initLoad ) { return; }

		let idBase = idToggle.substring(0,idToggle.indexOf('-area-toggle')); // = lib4ri-journal
		let idArea = idToggle.substring(idToggle.lastIndexOf('-')+1);		// = 12345

		let ord = location.search.indexOf('dev-test=');		// just for test reason
		ord = ( ( ord > 0 ) ? location.search.charCodeAt(ord+9) : 0 );

		if ( ord < 0 ) {
			// not used, data available+added right after search results will be displayed
			lib4riSearchJournalDetail( issnList, almaId, 'swisscovery', 'available,linklist', idBase, idArea );
		}
		if ( !ord || ord == 97 ) {
			lib4riSearchJournalDetail( issnList, almaId, 'api4ri', 'jif,agreement', idBase, idArea );
		}
		if ( !ord || ord == 98 ) {
			lib4riSearchJournalDetail( issnList, almaId, 'romeo', 'publisher,issn,eissn,embargo-fund', idBase, idArea );
		}
		if ( !ord || ord == 99 ) {
			lib4riSearchJournalDetail( issnList, almaId, 'doaj', 'publisher,issn,eissn,title,linklist,embargo-fund', idBase, idArea );
		}
	}
}

if ( typeof(lib4riJournalAreaShowAll) != 'function' ) {
	function lib4riJournalAreaShowAll() {
		let bbCount = 0;
		let bbElemAry = document.getElementsByClassName('lib4ri-journal-area-toggle-link');
		for(b=0;b<bbElemAry.length;b++) {
			// bbElemAry[b].id is for example: lib4ri-journal-area-toggle-12345
			let idToggle = bbElemAry[b].id;
			setTimeout( function() {
				let bbElem = document.getElementById(idToggle);
				let jArea = document.getElementById(idToggle.replace('-toggle-','-info-'));	// to get: lib4ri-journal-area-info-12345
				if ( bbElem && jArea && jArea.style.display == 'none' ) { bbElem.click(); }
			}, ( bbCount * 350 ) );
			bbCount++;
		}
	}
}

// Add search form + show the intended tab when the page is loaded:
// lib4riSearchFormAdd();
// lib4riSearchTabToggle();
