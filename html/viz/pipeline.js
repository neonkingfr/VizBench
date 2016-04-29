var pipenum;
var pipelinename;
var autosave = true;

function changeval(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	valueid.value = rangeid.value;
}

function sendparamchange(name) {
	var valueid = document.getElementById("value_"+name);
	var a = name.split(".");
	var viztag = pipenum + ":" + a[0];
	var paramname = a[1];
	var paramval = valueid.value;
	checkapi(vizapi("ffff.ffglparamset","{\"viztag\":\""+viztag+"\", \"param\":\""+paramname+"\", \"val\":\""+paramval+"\" }"));
}

function updateval(name) {
	changeval(name);
	sendparamchange(name);
	autosave_pipeline_norefresh();
}

function change_enable(vtag) {
	var enabledid = document.getElementById("enabled_"+vtag);
	var onoff = enabledid.checked ? "1" : "0";
	var fullviztag = "" + pipenum + ":" + vtag;
	checkapi(vizapi("ffff.ffglenable","{\"viztag\":\""+fullviztag+"\", \"onoff\": "+onoff+" }"));
	autosave_pipeline_norefresh();
}

function changemoveable(vtag) {
	var moveableid = document.getElementById("moveable_"+vtag);
	var onoff = moveableid.checked ? "1" : "0";
	var fullviztag = "" + pipenum + ":" + vtag;
	checkapi(vizapi("ffff.ffglmoveable","{\"viztag\":\""+fullviztag+"\", \"onoff\": "+onoff+" }"));
	autosave_pipeline_norefresh();
}

// updateval2 propagate changes from value_ to range_ rather
// than range_ to value_
function updateval2(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	rangeid.value = valueid.value;
	sendparamchange(name);
	autosave_pipeline_norefresh();
}

function moveplugin(fullviztag,n) {
	checkapi(vizapi("ffff.moveplugin","{\"viztag\":\""+fullviztag+"\", \"n\":\""+n+"\"}"));
	autosave_pipeline();
}

function browsepipelinefile() {
	var pipelinefile = document.getElementById("pipelinefile");
	pipelinefile.click();
}

function load_pipeline() {
	checkapi(vizapi("ffff.load_pipeline","{\"name\":\""+pipelinename+"\", \"pipenum\":\""+pipenum+"\" }"));
}

function save_pipeline() {
	var api = vizapi("ffff.save_pipeline","{\"name\":\""+pipelinename+"\", \"pipenum\":\""+pipenum+"\" }");
	checkapi(api);
}

function autosave_pipeline_norefresh() {
	autosave_pipeline_(false);
}

function autosave_pipeline() {
	autosave_pipeline_(true);
}

function autosave_pipeline_(refresh) {
	// var autosave = document.getElementById("autosave").checked;
	if ( autosave&& pipelinename != "" ) {
		save_pipeline()
		if ( refresh ) {
			pipeline_pagegen(pipenum,pipelinename);
		}
	}
}

function shufflepipeline() {
	if ( checkapi(vizapi("ffff.shufflepipeline","{\"pipenum\":\""+pipenum+"\" }")) ) {
		autosave_pipeline();
	}
}

function randomizepipeline() {
	if ( checkapi(vizapi("ffff.randomizepipeline","{\"pipenum\":\""+pipenum+"\" }")) ) {
		autosave_pipeline();
	}
}

function ffffrecord(onoff) {
	checkapi(vizapi("ffff.record","{\"onoff\": "+onoff+" }"));
}

function enable_all(onoff) {
	var api = vizapi("ffff.ffglpipeline","{\"pipenum\":\""+pipenum+"\" }");
	if ( ! checkapi(api) ) {
		return;
	}
	var ffglpipeline = api.result;
	for ( var i=0; i<ffglpipeline.length; i++ ) {

		var f = ffglpipeline[i];
		// var fullviztag = "" + pipenum + ":" + f.viztag;
		var fullviztag = f.viztag;
		checkapi(vizapi("ffff.ffglenable","{\"viztag\":\""+fullviztag+"\", \"onoff\": "+onoff+" }"));
	}
	autosave_pipeline();
}

function changeTextVal_keypress(event,tag,api,argname) {
	if ( event.keyCode != 13 ) {
		return;
	}
	changeTextVal(tag,api,argname);
}

function changeTextVal(tag,api,argname) {
	var inputid = document.getElementById(tag+"_"+api);
	var inputvalue = inputid.value;
	var api = vizapi(tag+"."+api,"{\""+argname+"\":\""+inputvalue+"\"}");
	if ( ! checkapi(api) ) {
		inputid.value = "";
		return;
	}
	autosave_pipeline();
}

function params_class_of_api(api) {
	var p;
	if ( api.search("midi") >= 0 ) {
		p = "midi";
	} else if ( api.search("sprite") >= 0 ) {
		p = "sprite";
	} else {
		p = "UNKNOWN";
	}
	return p;
}

function pipeline_pagegen(pn,fname) {

	pipenum = pn;
	pipelinename = fname;

	var api = vizapi("ffff.ffglpipeline","{\"pipenum\":\""+pipenum+"\"}");
	if ( ! checkapi(api) ) {
		return;
	}
	var ffglpipeline = api.result;

	var html = "";

	html += "Plugins: &nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Enable All\" onClick=\"enable_all(1);\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Disable All\" onClick=\"enable_all(0);\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Shuffle\" onClick=\"shufflepipeline();\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Randomize\" onClick=\"randomizepipeline();\">";

	var r;
	html += "<table>";	// two columns, left column for plugin name
				// and misc. buttons, and the right column
				// for parameters
				
	for ( var i=0; i<ffglpipeline.length; i++ ) {

		var f = ffglpipeline[i];

		// Remove the pipenum value from the viztag.
		// By convention, vtag is the viztag without the pipenum
		var vtag = f.viztag.substr(2);

		var fullviztag = f.viztag;

		html += "<tr><td colspan=10><hr></td></tr>\n";
		html += "<tr><td valign=top>";	// first column of outer table

		var enabledid = "enabled_"+vtag;
		var enabledchecked = "";
		if ( f.enabled ) {
			enabledchecked = "checked=\"checked\"";
		}

		var moveableid = "moveable_"+vtag;
		var moveablechecked = "";
		if ( f.moveable ) {
			moveablechecked = "checked=\"checked\"";
		}

		html += "<table>";
		var parenstuff = "";
		if ( vtag != f.plugin ) {
			parenstuff = "("+f.plugin+")";
		}
		html += "<tr><td colspan=10><b><font style=\"font-size: 200%\">"+vtag+"</font>&nbsp;&nbsp;"+parenstuff+"</b></td></tr>";
		html += "<tr>";
		html += "<td width=60px>Enabled</td><td><input type=\"checkbox\" "+enabledchecked+" id=\""+enabledid+"\" onchange=\"change_enable('"+vtag+"')\")></td><td></td><td width=100px></td>";
		html += "</tr>";
		html += "<tr>";
		html += "<td width=60px>Moveable</td><td><input type=\"checkbox\" "+moveablechecked+" id=\""+moveableid+"\" onchange=\"changemoveable('"+vtag+"')\")></td>";
		html += "<td>&nbsp;Move";
		html += "&nbsp;<input type=\"button\" value=\"up1\" onClick=\"moveplugin('"+fullviztag+"',-1)\")>";
		html += "&nbsp;<input type=\"button\" value=\"up4\" onClick=\"moveplugin('"+fullviztag+"',-4)\")>";
		html += "&nbsp;<input type=\"button\" value=\"down4\" onClick=\"moveplugin('"+fullviztag+"',4)\")>";
		html += "&nbsp;<input type=\"button\" value=\"down1\" onClick=\"moveplugin('"+fullviztag+"',1)\")>";
		html += "</td></tr>";

		var api = vizapi("ffff.about","{\"viztag\":\""+fullviztag+"\"}");
		var about = api.result;
		var api = vizapi("ffff.description","{\"viztag\":\""+fullviztag+"\"}");
		var desc = api.result;

		html += "<tr height=10px></tr>";
		html += "<tr><td colspan=10><font style=\"font-size:90%\">Desc: "+desc+"</font></td></tr>";
		html += "<tr><td colspan=10><font style=\"font-size:90%\">About: "+about+"</font></td></tr>";

		html += "</table>";

		html += "</td><td>";

		if ( f.vizlet ) {
			var apis = vizapi(fullviztag+'.apis').result;
			if ( apis == "" ) {
				html += "<p>No APIs";
			} else {
				html += "<table>";
				html += "<tr><td colspan=10>Vizlet APIs:</td></tr>";
				html += "<tr height=10px></tr>";

				var a = apis.split(";");
				for ( var n=0; n<a.length; n++ ) {
					html += "<tr><td></td>";
					var w = a[n].split(/[\(\)]/);
					if ( w == "" ) {
						continue;
					}
					// We expect the length of w to be either 1 (just an api name) or
					// 3 (api name + argument)
					var api = w[0];
					var fullapiname = fullviztag + "." + api;
					if ( w.length <= 1 ) {
						html += "<td><input type=\"button\" style=\"width:100px\" value=\""+a[n]+"\" onClick=\"checkapi(vizapi('"+fullapiname+"'));\"></td>";
					} else if ( w.length != 3 ) {
						alert("Hey, too many arguments on api="+w[0]+" length="+w.length+" an="+a[n]);
					} else {
						var args = w[1].split(",");

						var argname = args[0];
						var inputid = fullviztag+"_"+api;
						var val = "";
						if ( /^set_/.test(api) ) {

							var api_sans_set = api.substr(4);
							var getapi = "get_"+api_sans_set;
							var aaa = vizapi(fullviztag+"."+getapi,"{\"viztag\":\""+fullviztag+"\"}");
							val = aaa.result;
							html += "<td align=right>"
								+api_sans_set+"&nbsp;"+argname+"&nbsp;=&nbsp;</td><td><input type=\"text\" onKeypress=\"changeTextVal_keypress(event,'"+fullviztag+"','"+api+"','"+argname+"')\" value=\""+val+"\" id=\""+inputid+"\" size=20></td>";
							if ( argname == "paramfile" ) {
								var paramsclass = params_class_of_api(api)
								html += "<td>";
								html += "<button onclick=\"changeTextVal('"+fullviztag+"','"+api+"','"+argname+"');edit_params('"+inputid+"','"+paramsclass+"');\" >Edit</button>";
								html += "&nbsp;&nbsp;";
								html += "<button onclick=\"newvariation_params('"+inputid+"','"+paramsclass+"','"+fullviztag+"','"+api+"','"+argname+"');\" >New Variation</button>";
							}

						} else {
							html += "<td><input type=\"button\" style=\"width:150px\" value=\""
								+api+"\" onClick=\"checkapi(vizapi('"+fullapiname+"','{}'));\"></td>";

						}

					}
					html += "</tr>";
				}
				html += "</table>";
			}
		}
		html += "</td></tr>";

		html += "<tr><td></td><td>"

		// var fullviztag = "" + pipenum + ":" + f.viztag;
		var fullviztag = f.viztag;
		var api = vizapi("ffff.ffglparamvals","{\"viztag\":\""+fullviztag+"\"}");
		if ( ! checkapi(api) ) {
			continue;
		}
		var vals = api.result;

		html += "<table>";   // for all the parameters
		var did_header = false;
		for ( var n=0; n<vals.length; n++ ) {
			var v = vals[n];

			var name = vtag+"."+v.name;
			var valueid = "value_"+name;
			var rangeid = "range_"+name;

			if ( v.name == "viztag" ) {
				// silently ignore the viztag parameter
				continue;
			}
			if ( v.type == "text" ) {
				alert("Unable to handle text parameter - "+v.name);
				continue;
			}

			if ( ! did_header ) {
				html += "<tr><td colspan=10>FFGL Parameters:</td></tr>";
				did_header = true;
			}
			html += "<tr><td width=15%></td>";

			var argstr = "&quot;viztag&quot; : "
				+ "&quot;'+document.getElementById(&quot;" + valueid + "&quot;).value+'&quot;"
				;

			html += "<td align=right width=25%>"+v.name+"</td>";

			var mn = 0.0;
			var mx = 1.0;
			var stepsize = 0.01;

			html += "<td width=10></td><td width=25%><input style=\"width:98%\" value=\""+v.value+"\" type=\"number\" min=\""+mn+"\" max=\""+mx+"\" id=\""+valueid+"\" size=8 onchange=\"updateval2('"+name+"');\" ></td>";

			html += "<td><input style=\"width:90%\" id=\""+rangeid+"\" type=\"range\" value=\""+v.value+"\" min=\""+mn+"\" max=\""+mx+"\" step=\""+stepsize+"\" oninput=\"changeval('"+name+"');\" onchange=\"updateval('"+name+"');\" ></td>";

			html += "</tr>";
		}
		html += "</table>";  	// end of parameter table
		html += "</td></tr>";	// end of row for outer table
	}
	html += "</table>";  // end of outer table
	html += "<br><hr>\n";

	document.getElementById("pipeline").innerHTML = html;
}

function newvariation_params(inputid,paramsclass,viztag,api,argname) {

	var oldfilename = dojo.byId(inputid).value;
	var ix = oldfilename.indexOf("-");
	var newfilename;
	if ( ix > 0 ) {
		// There's already a variation number in the filename,
		// increment it
		var num = 1 + parseInt(oldfilename.substr(ix+1));
		newfilename = oldfilename.substr(0,ix) + "-" + num;
		
	} else {
		newfilename = oldfilename + "-1";
	}

	var copyapi = "ffff.copy_"+paramsclass;

	alert("copying from="+oldfilename+" to="+newfilename);
	checkapi(vizapi(copyapi,"{\"fromfile\":\""+oldfilename+"\", \"tofile\":\""+newfilename+"\", \"pipenum\":\""+pipenum+"\" }"));
	alert("AFTER copying from="+oldfilename+" to="+newfilename);

	dojo.byId(inputid).value = newfilename;

	changeTextVal(viztag,api,argname);

	window.location.href = ("edit_" + paramsclass + "_params.html?paramfile="+newfilename);
}
