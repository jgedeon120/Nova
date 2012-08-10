//============================================================================
// Name        : NovaGrid.js
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Simple grid for displaying suspects and other web UI stuff
//============================================================================

// Constructor
//   columns: array of column objects
//     Each should contain a "name" attribute and optionally a "formatter"
//   keyIndex: index of column used as a UID for a row
//   tableElement: DOM object of the <table> 
var NovaGrid = function(columns, keyIndex, tableElement) {
	this.m_columns = columns;
	this.m_keyIndex = keyIndex;
	this.m_sortByKey = keyIndex;
    this.m_sortDescending = true;
	this.m_tableElement = tableElement;
	this.m_elements = new Object();

	this.GenerateTableHeader();
}

NovaGrid.prototype = {
    // Adds a new row to the table (or updates row if it exists with same keyIndex)
	PushEntry: function (entry) {
		if (entry.length != this.m_columns.length) {
			throw "Can't push entry of size " + entry.length + " into table of size " + this.m_columns.length
		} else {
			this.m_elements[entry[this.m_keyIndex]] = entry;
		}
	}
    
    // Used internally to generate the TH tags
	, GenerateTableHeader: function() {
		this.headerHTML = "";
		this.headerHTML += '<TR>';
		for (var c = 0; c < this.m_columns.length; c++) {
			var title = this.m_columns[c].name;

			if (this.m_sortByKey == c) {
				title = '<div class="sortArrow">' + (this.m_sortDescending ? '&#8744;' : '&#8743;') + '</div>' + title;
			}
			this.headerHTML += '<TH><A HREF="javascript:void(0)" onclick="suspectGrid.SetSortByKey(' + c + ');">' + title + '</A></TH>';
		}
		this.headerHTML += '</TR>';
	}

    // Returns the HTML for the table
	, GetTable: function() {
		var innerTableString = this.headerHTML;
	
		var keys = Object.keys(this.m_elements);
		var arrayRep = new Array();
		for (var i = 0; i < keys.length; i++) {
			arrayRep.push(this.m_elements[keys[i]]);
		}

		// work around for scoping issues
		var so = this;
		
		arrayRep.sort(function(a, b) {
			if (a[so.m_sortByKey] > b[so.m_sortByKey]) {
                return so.m_sortDescending == true ? -1 : 1;
			} else if (a[so.m_sortByKey] < b[so.m_sortByKey]) {
                return so.m_sortDescending == true ? 1 : -1;
			} else {
				return 0;
			}
		});

		for (var i = 0; i < arrayRep.length; i++) {
			innerTableString += '<TR>';
			for (var c = 0; c < this.m_columns.length; c++) {
				if (this.m_columns[c].formatter !== undefined) {
				   innerTableString += '<TD>' + this.m_columns[c].formatter(arrayRep[i][c]) + '</TD>';
				} else {
					innerTableString += '<TD>' + arrayRep[i][c] + '</TD>';
				}
			}
			innerTableString += '</TR>';
		}

		return innerTableString;
	}

    // Sets which column to sort the table by
	, SetSortByKey: function(key) {
        if (this.m_sortByKey == key) {
            this.m_sortDescending = !this.m_sortDescending;
        }
		this.m_sortByKey = key;
		this.GenerateTableHeader();
		this.Render();
	}

    // Clears all elements from the table
	, Clear: function() {
		this.m_elements = new Array();
		this.Render();
	}

	, DeleteRow: function(key) {
		delete this.m_elements[key];
	}

    // Returns the number of rows in the table
	, Size: function() {
		return Object.keys(this.m_elements).length;
	}

    // Renders the table
	, Render: function() {
		// Simple way (slow in Chrome, fine in FF)
		//theDoc.getElementById("suspectTable").innerHTML = suspectGrid.GetTable();
		
		this.m_tableElement = replaceHtml(this.m_tableElement, this.GetTable());
	}
}

// This is an ugly hack which happens to double performance in Chrome for the suspect grid. No diff in firefox
// from here, http://blog.stevenlevithan.com/archives/faster-than-innerhtml
function replaceHtml(el, html) {
	var oldEl = typeof el === "string" ? document.getElementById(el) : el;
	/*@cc_on // Pure innerHTML is slightly faster in IE
	  oldEl.innerHTML = html;
	  return oldEl;
	  @*/
	var newEl = oldEl.cloneNode(false);
	newEl.innerHTML = html;
	oldEl.parentNode.replaceChild(newEl, oldEl);
	/* Since we just removed the old element from the DOM, return a reference
	   to the new element, which can be used to restore variable references. */
	return newEl;
};

