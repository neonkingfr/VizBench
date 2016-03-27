ParamList = null;
StringVals = {};
ParamsClass = "undefined"

var paramfilename;

var autosave = true;

function updateonevalue(name,value) {
	var obj = ParamList[name];
	var type = obj["type"];
	var valueid = document.getElementById("value_"+name);
	if ( valueid ) {
		valueid.value = value;
	}
	if ( type == "double" || type == "int" ) {
		var rangeid = document.getElementById("range_"+name);
		if ( rangeid ) {
			rangeid.value = value;
		}
	} else if ( type == "bool" ) {
	} else if ( type == "string" ) {
	}
}

function readfile() {
	var filename = paramfilename;
	if ( filename == "" ) {
		set_status("No Filename set, can't Load!");
		return;
	}
	var api = vizapi('VizServer.'+ParamsClass+'param_readfile','{ "paramfile":"'+filename+'" }');
	if ( checkapi(api) ) {
		var j = JSON.parse(api.result);
		if ( j ) {
			for ( var name in j ) {
				var value = j[name];
				// Make sure it's a valid value, if a string
				var obj = ParamList[name];
				if ( obj == null ) {
					alert("No attribute named "+name+"?");
					continue;
				}
				var type = obj["type"];
				if ( type=="string" && stringvalindex(name,value) < 0 ) {
					// default to first value
					value = StringVals[name][0];
				}
				updateonevalue(name,value);
			}
			// For any ParamList values that aren't in the file,
			// set them to default values.
			for ( var name in ParamList ) {
				var obj = ParamList[name];
				var valueid = document.getElementById("value_"+name);
				if ( valueid.value == "" ) {
					updateonevalue(name,obj["default"]);
				}
			}
			set_status("");
		} else {
			set_status("Unable to interpret JSON from VizServer");
		}
	}
}

function randparams() {
	for ( var name in ParamList ) {
		var obj = ParamList[name];
		var type = obj["type"];
		var mn = obj["min"];
		var mx = obj["max"];
		var randableid = document.getElementById("randable_"+name);
		var randminid = document.getElementById("randmin_"+name);
		var randmaxid = document.getElementById("randmax_"+name);

		if ( ! randableid || ! randableid.checked ) {
			continue;
		}

		var v;
		if ( type == "bool" ) {
			v = (Math.random() < 0.5);
		} else if ( type == "string" ) {
			var valarray = StringVals[name];
			i = Math.random() * (valarray.length+1);
			i = Math.floor(i) % valarray.length;
			v = valarray[i];
		} else {
			if ( !randminid || !randmaxid ) {
				continue;
			}
			var rmn = randminid.value;
			var rmx = randmaxid.value;
			if ( rmn == "" ) {
				rmn = mn;
			}
			if ( rmx == "" ) {
				rmx = mx;
			}
			rmn = Number(rmn);
			rmx = Number(rmx);
			v = rmn + Math.random() * (rmx-rmn);
		}
		updateonevalue(name,v);
	}
	autowritefile();
}

function setallrandable(onoff) {
	for ( var name in ParamList ) {
		var randableid = document.getElementById("randable_"+name);
		if ( !randableid ) {
			continue;
		}
		randableid.checked = onoff;
	}
}

// Not really useful at the moment, I've removed the button for this
function randdefaults() {
	for ( var name in ParamList ) {
		var obj = ParamList[name];
		var type = obj["type"];
		var mn = obj["min"];
		var mx = obj["max"];
		var randableid = document.getElementById("randable_"+name);
		var randminid = document.getElementById("randmin_"+name);
		var randmaxid = document.getElementById("randmax_"+name);
		if ( !randableid || !randminid || !randmaxid ) {
			continue;
		}
		randminid.value = mn;
		randmaxid.value = mx;
		continue;

		var rmn = randminid.value;
		var rmx = randmaxid.value;
		if ( rmn == "" ) {
			rmn = mn;
		}
		if ( rmx == "" ) {
			rmx = mx;
		}
		rmn = Number(rmn);
		rmx = Number(rmx);
		if ( randableid.checked ) {
			var v = rmn + Math.random() * (rmx-rmn);
			updateonevalue(name,v);
		}
	}
}

function loaddefaults() {
	if ( ParamList == null ) {
		set_status("No ParamList value!?");
		return;
	}
	for ( var name in ParamList ) {
		updateonevalue(name,ParamList[name]["default"]);
	}
	set_status("");
	autowritefile();
}

function writefile() {
	var contents = "";
	var sep = "{ ";
	for ( name in ParamList ) {
		var valueid = document.getElementById("value_"+name);
		contents += (sep+"\""+name+"\":\""+valueid.value+"\"");
		sep = ",";
	}
	contents += " }";
	var api = vizapi('VizServer.'+ParamsClass+'param_writefile','{ "paramfile":"'+paramfilename+'", "contents":'+contents+' }');
	if ( paramfilename == "" ) {
		set_status("No Filename set, can't Save!");
	} else if ( api.result == "" ) {
		// set_status("File "+paramfilename+" has been updated");
		set_status("");
	} else {
		set_status(api.error);
	}
}

function browsefile() {
	var paramfile = document.getElementById("paramfile");
	paramfile.click();
}

function togglebool(name) {
	var valueid = document.getElementById("value_"+name);
	if ( valueid.value == "true" || valueid.value == "True" || valueid.value == "1") {
		valueid.value = "false";
	} else {
		valueid.value = "true";
	}
	autowritefile();
}

function stringvalindex(name,val) {
	var valarray = StringVals[name];
	for ( var i=0; i<valarray.length; i++ ) {
		if ( val == valarray[i] ) {
			return i;
		}
	}
	return -1;
}

function autowritefile() {
	// var autosave = document.getElementById("autosave").checked;
	var paramfilename = document.getElementById("paramfilename");
	if ( autosave && paramfilename != "" ) {
		writefile()
	}
}

function nextvalue(name,increment) {
	var valueid = document.getElementById("value_"+name);
	var curr = valueid.value;
	var valarray = StringVals[name];
	var i = stringvalindex(name,curr);
	if ( i<0 || i >= valarray.length ) {
		// not found
		curr = valarray[0];
	} else {
		i += increment;
		if ( i < 0 ) {
			i = valarray.length - 1;
		} else if ( i >= valarray.length ) {
			i = 0;
		}
		curr = valarray[i];
	}
	valueid.value = curr;
	autowritefile();
}

function changeval(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	valueid.value = rangeid.value;
}

function updateval(name) {
	changeval(name);
	autowritefile();
}

// updateval2 propagate changes from value_ to range_ rather
// than range_ to value_
function updateval2(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	rangeid.value = valueid.value;
	autowritefile();
}

// paramsclass is either "sprite" or "midi"
function make_edit_page(paramsclass) {

	var pipenum = 0;

	// Fill in the Filename with the paramfile query value
	var vals = queryvalues();
	if ( vals["paramfile"] ) {
		paramfilename = vals["paramfile"];
	}
	if ( vals["pipenum"] ) {
		pipenum = vals["pipenum"];
	}

	ParamsClass = paramsclass

	var params = document.getElementById("params");

	var host = document.location.host;
	var i = host.indexOf(":");
	var instancenum = 1;
	if ( i > 0 ) {
		var port = host.substr(i+1);
		instancenum = port - 4439;
	}

	var typetitle = ParamsClass.substr(0,1).toUpperCase() + ParamsClass.substr(1);

	if ( ! pipenum ) {
		pipenum = 0;
	}

	var otherlinks = "<a href=\"pipeline.html?pipenum="+pipenum+"\">Back to Pipeline</a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"ffff.html\">FFFF Home</a>";
	titlegen(typetitle+" - "+paramfilename,otherlinks);

	var html = "";

	// html += "<input type=\"button\" style=\"width:100px\" value=\"Load\" onClick=\"readfile();\">";
	// html += "&nbsp;&nbsp;";

	// html += "<input type=\"button\" style=\"width:100px\" value=\"Defaults\" onClick=\"loaddefaults();\">";
	// html += "&nbsp;&nbsp;";

	html += "<input type=\"button\" style=\"width:150px\" value=\"Randomize\" onClick=\"randparams();\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" style=\"width:150px\" value=\"Set ALL Randomize\" onClick=\"setallrandable(true);\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" style=\"width:150px\" value=\"Unset ALL Randomize\" onClick=\"setallrandable(false);\">";
	// html += "&nbsp;&nbsp;";
	// html += "<input type=\"button\" style=\"width:100px\" value=\"Rand Defaults\" onClick=\"randdefaults();\">";
	html += "&nbsp;&nbsp;";

	// html += "<input type=\"button\" style=\"width:100px\" value=\"Save\" onClick=\"writefile();\">";
	// html += "&nbsp;&nbsp;";
	// html += "AutoSave<input type=\"checkbox\" checked=\"checked\" id=\"autosave\" value=\"AutoSave\" >";

	html += "<p><hr>";

	var api = vizapi('VizServer.'+ParamsClass+'param_list');    // e.g. it calls VizServer.spriteparam_list
	if ( checkapi(api) ) {
		html += "<table width=100% border=0>";
		html += "<tr><td>Param</td><td colspan=2 align=center>Value</td><td>Rand?</td><td align=center>Rand Min</td><td align=center>Rand Max</td></tr>";
		var j = JSON.parse(api.result);
		if ( ! j ) {
			set_status("Unable to parse VizServer.param_list result as JSON?");
			ParamList = null;
		} else {
		    ParamList = j;
		    for ( var name in ParamList ) {
			if ( ! ParamList.hasOwnProperty(name) ) {
				continue;
			}
			html += "<tr>";
			var obj = ParamList[name];
			var type = obj["type"];
			var mn = obj["min"];
			var mx = obj["max"];
			var stepsize = (mx-mn)/200.0;

			html += "<td>"+name+"</td>";
			var valueid = "value_"+name;
			if ( type == "int" || type == "double" ) {
				html += "<td><input style=\"width:98%\" id=\""+valueid+"\" type=\"number\" min=\""+mn+"\" max=\""+mx+"\" onchange=\"updateval2('"+name+"');\" ></td>";
			} else if (type == "bool" ) {
				html += "<td><input style=\"width:98%\" id=\""+valueid+"\" type=\"text\" ></td>";
			} else if ( type == "string" ) {
				html += "<td><input style=\"width:98%\" id=\""+valueid+"\" type=\"text\" ></td>";
			}
			// html += "<td>"+mn+"</td><td>"+mx+"</td>";
			html += "<td>";

			var rangeid = "range_"+name
			if ( type == "int" ) {
				html += "<input id=\""+rangeid+"\" type=\"range\" min=\""+mn+"\" max=\""+mx+"\" oninput=\"changeval('"+name+"');\" onchange=\"updateval('"+name+"');\" >";
			} else if ( type == "double" ) {
				html += "<input id=\""+rangeid+"\" type=\"range\" min=\""+mn+"\" max=\""+mx+"\" step=\""+stepsize+"\" oninput=\"changeval('"+name+"');\" onchange=\"updateval('"+name+"');\" >";
			} else if ( type == "bool" ) {
				html += "<button style=\"width:98%\" onclick=\"togglebool('"+name+"');\" >Toggle</button>";
			} else if ( type == "string" ) {
				// For strings, the min/max values convey
				// the string values type.
				var valstype = mn;
				var a = vizapi('VizServer.'+ParamsClass+'param_stringvals','{ "type":"'+valstype+'" }');
				if ( a.result != "" ) {
					var j = JSON.parse(a.result);
					if ( !j ) {
						alert("Unable to parse ...param_stringvals output? result="+a.result);
					} else {
						StringVals[name] = j;
					}
					html += "<button style=\"width:45%\" onclick=\"nextvalue('"+name+"',-1);\" >Prev</button>";
					html += "&nbsp;";
					html += "<button style=\"width:45%\" onclick=\"nextvalue('"+name+"',1);\" >Next</button>";
				} else {
					set_status(a.error);
					html += "<b>Error in param_stringvals</b>";
				}
			}
			html += "</td>";
			var randableid = "randable_"+name;
			html += "<td align=center><input type=\"checkbox\" id=\""+randableid+"\" ></td>";
			var randminid = "randmin_"+name;
			var randmaxid = "randmax_"+name;
			if ( type == "int" ) {
				html += "<td><input style=\"width:98%\" id=\""+randminid+"\" type=\"number\" min=\""+mn+"\" max=\""+mx+"\" ></td>";
				html += "<td><input style=\"width:98%\" id=\""+randmaxid+"\" type=\"number\" min=\""+mn+"\" max=\""+mx+"\" ></td>";
			} else if ( type == "double" ) {
				html += "<td><input style=\"width:98%\" id=\""+randminid+"\" type=\"number\" min=\""+mn+"\" max=\""+mx+"\" ></td>";
				html += "<td><input style=\"width:98%\" id=\""+randmaxid+"\" type=\"number\" min=\""+mn+"\" max=\""+mx+"\" ></td>";
			} else if (type == "bool" ) {
			} else if ( type == "string" ) {
			}
			html += "</tr>";
		    }
		}
	}

	params.innerHTML = html;

	readfile();
}
