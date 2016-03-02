status = null;

function changeval(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	valueid.value = rangeid.value;
}

function sendparamchange(name) {
	var valueid = document.getElementById("value_"+name);
	var a = name.split(".");
	var viztag = a[0];
	var paramname = a[1];
	var paramval = valueid.value;
	var api = vizapi("ffff.ffglparamset","{\"viztag\":\""+viztag+"\", \"param\":\""+paramname+"\", \"val\":\""+paramval+"\" }");
	checkapi(api);
}

function updateval(name) {
	changeval(name);
	sendparamchange(name);
}

function changeenable(viztag) {
	var enabledid = document.getElementById("enabled_"+viztag);
	var onoff = enabledid.checked ? "1" : "0";
	var a = vizapi("ffff.ffglenable","{\"viztag\":\""+viztag+"\", \"onoff\": "+onoff+" }");
	checkapi(api);
}

function changemoveable(viztag) {
	var moveableid = document.getElementById("moveable_"+viztag);
	var onoff = moveableid.checked ? "1" : "0";
	var a = vizapi("ffff.ffglmoveable","{\"viztag\":\""+viztag+"\", \"onoff\": "+onoff+" }");
	checkapi(api);
}

// updateval2 propagate changes from value_ to range_ rather
// than range_ to value_
function updateval2(name) {
	var valueid = document.getElementById("value_"+name);
	var rangeid = document.getElementById("range_"+name);
	rangeid.value = valueid.value;
	sendparamchange(name);
}

function checkapi(api) {
	if ( api.error != "" ) {
		alert("error="+api.error);
		status.innerHTML = api.error;
		return false;
	} else {
		status.innerHTML = "OK!";
		return true;
	}
}

function moveup(viztag) {
	var api = vizapi("ffff.moveup","{\"viztag\":\""+viztag+"\"}");
	checkapi(api);
	ffffpagegen();
}

function movedown(viztag) {
	var api = vizapi("ffff.movedown","{\"viztag\":\""+viztag+"\"}");
	checkapi(api);
	ffffpagegen();
}

function browsepipelinefile() {
	var pipelinefile = document.getElementById("pipelinefile");
	pipelinefile.click();
}

function changepipelinefile() {
	var pipelinefilename = document.getElementById("pipelinefilename");
	var pipelinefile = document.getElementById("pipelinefile");
	var val = pipelinefile.value;
	var words = val.split(/[\\\/]/);
	if ( words.length > 1 ) {
		val = words[words.length-1];
	}
	pipelinefilename.value = val;
	loadpipeline();
}

function refreshpipelinefilename() {
	var api = vizapi("ffff.pipelinefilename");
	if ( api.error != "" ) {
		status.innerHTML = api.error;
		pipelinefilename = "";
	} else {
		pipelinefilename = api.result;
	}
	if ( pipelinefilename != "" ) {
		document.getElementById("pipelinefilename").value = pipelinefilename;
	}
}

function loadpipeline() {
	var fname = document.getElementById("pipelinefilename").value;
	var api = vizapi("ffff.loadpipeline","{\"filename\":\""+fname+"\"}");
	status = document.getElementById("status");
	checkapi(api);
	ffffpagegen();
	// refreshpipelinefilename();
}

function savepipeline() {
	var fname = document.getElementById("pipelinefilename").value;
	var api = vizapi("ffff.savepipeline","{\"filename\":\""+fname+"\"}");
	if ( checkapi(api) ) {
		status.innerHTML = "OK - pipeline saved";
	}
}

function shufflepipeline() {
	var api = vizapi("ffff.shufflepipeline");
	if ( checkapi(api) ) {
		ffffpagegen();
	}
}

function randomizepipeline() {
	var api = vizapi("ffff.randomizepipeline");
	if ( checkapi(api) ) {
		ffffpagegen();
	}
}

function ffffrecord(onoff) {
	var api = vizapi("ffff.record","{\"onoff\": "+onoff+" }");
	checkapi(api);
}

function enableall(onoff) {
	var api = vizapi("ffff.ffglpipeline");
	if ( ! checkapi(api) ) {
		status.innerHTML = api.error;
		return;
	}
	var ffglpipeline = api.result;
	for ( var i=0; i<ffglpipeline.length; i++ ) {

		var f = ffglpipeline[i];
		var a = vizapi("ffff.ffglenable","{\"viztag\":\""+f.viztag+"\", \"onoff\": "+onoff+" }");
	}
	ffffpagegen();
}

function changeTextVal(tag,api,argname) {
	var inputid = document.getElementById(tag+"_"+api);
	var api = vizapi(tag+"."+api,"{\""+argname+"\":\""+inputid.value+"\"}");
	if ( ! checkapi(api) ) {
		status.innerHTML = api.error;
		inputid.value = "";
		return;
	}
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

function pipeline_pagegen() {

	var api = vizapi("ffff.ffglpipeline");
	if ( ! checkapi(api) ) {
		status.innerHTML = api.error;
		return;
	}
	var ffglpipeline = api.result;

	var html = "";

	html += "<table>";
	html += "<tr><td>Status:</td><td><div id=\"status\">OK</div></td></tr>";
	html += "<tr><td>";
	html += "<input type=\"file\" style=\"display: none\" id=\"pipelinefile\" onChange=\"changepipelinefile();\" >";

	html += "Filename: ";
	html += "</td><td>";
	html += "<input type=\"text\" style=\"width:300px\" id=\"pipelinefilename\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" style=\"width:100px\" value=\"Browse\" onClick=\"browsepipelinefile();\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" style=\"width:100px\" value=\"Load\" onClick=\"loadpipeline();\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" style=\"width:100px\" value=\"Save\" onClick=\"savepipeline();\">";
	html += "</td>";
	html += "</table>";

	html += "<p>";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Enable All\" onClick=\"enableall(1);\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Disable All\" onClick=\"enableall(0);\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Shuffle Pipeline\" onClick=\"shufflepipeline();\">";
	html += "&nbsp;&nbsp;";
	html += "<input type=\"button\" value=\"Randomize Pipeline\" onClick=\"randomizepipeline();\">";

	// html += "&nbsp;&nbsp;";
	// html += "<input type=\"button\" value=\"Randomize FFGL Params\" onClick=\"randomizeffglparams();\">";


	var r;
	html += "<table>";	// two columns, left column for plugin name
				// and misc. buttons, and the right column
				// for parameters
				
	for ( var i=0; i<ffglpipeline.length; i++ ) {

		var f = ffglpipeline[i];

		html += "<tr><td colspan=10><hr></td></tr>\n";
		html += "<tr><td valign=top>";	// first column of outer table

		var enabledid = "enabled_"+f.viztag;
		var enabledchecked = "";
		if ( f.enabled ) {
			enabledchecked = "checked=\"checked\"";
		}

		var moveableid = "moveable_"+f.viztag;
		var moveablechecked = "";
		if ( f.moveable ) {
			moveablechecked = "checked=\"checked\"";
		}

		html += "<table>";
		var parenstuff = "";
		if ( f.viztag != f.plugin ) {
			parenstuff = "("+f.plugin+")";
		}
		html += "<tr><td colspan=10><b><font style=\"font-size: 200%\">"+f.viztag+"</font>&nbsp;&nbsp;"+parenstuff+"</b></td></tr>";
		html += "<tr>";
		html += "<td width=60px>Enabled</td><td><input type=\"checkbox\" "+enabledchecked+" id=\""+enabledid+"\" onchange=\"changeenable('"+f.viztag+"')\")></td><td></td><td width=100px></td>";
		html += "</tr>";
		html += "<tr>";
		html += "<td width=60px>Moveable</td><td><input type=\"checkbox\" "+moveablechecked+" id=\""+moveableid+"\" onchange=\"changemoveable('"+f.viztag+"')\")></td>";
		html += "<td>&nbsp;Move";
		html += "&nbsp;<input type=\"button\" value=\"up\" onClick=\"moveup('"+f.viztag+"')\")>";
		html += "&nbsp;<input type=\"button\" value=\"down\" onClick=\"movedown('"+f.viztag+"')\")>";
		html += "</td></tr>";

		var api = vizapi("ffff.about","{\"viztag\":\""+f.viztag+"\"}");
		var about = api.result;
		var api = vizapi("ffff.description","{\"viztag\":\""+f.viztag+"\"}");
		var desc = api.result;

		html += "<tr height=10px></tr>";
		html += "<tr><td colspan=10><font style=\"font-size:90%\">Desc: "+desc+"</font></td></tr>";
		html += "<tr><td colspan=10><font style=\"font-size:90%\">About: "+about+"</font></td></tr>";

		html += "</table>";

		html += "</td><td>";

		if ( f.vizlet ) {
			var tag = f.viztag;
			var apis = vizapi(tag+'.apis').result;
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
					var fullapiname = tag + "." + api;
					if ( w.length <= 1 ) {
						html += "<td><input type=\"button\" style=\"width:100px\" value=\""+a[n]+"\" onClick=\"vizapi('"+fullapiname+"');\"></td>";
					} else if ( w.length != 3 ) {
						alert("Hey, too many arguments on api="+w[0]+" length="+w.length+" an="+a[n]);
					} else {
						var args = w[1].split(",");

						var argname = args[0];
						var inputid = tag+"_"+api;
						var val = "";
						if ( /^set_/.test(api) ) {

							// var argstr = "&quot;" + argname + "&quot; : "
							// 	+ "&quot;'+document.getElementById(&quot;" + inputid + "&quot;).value+'&quot;";
							//
							// html += "<td><input type=\"button\" style=\"width:150px\" value=\""
							// 	+api+"\" onClick=\"vizapi('"+fullapiname+"','{"+argstr+"}');\"></td>";

							var api_sans_set = api.substr(4);
							var getapi = "get_"+api_sans_set;
							var aaa = vizapi(tag+"."+getapi,"{\"viztag\":\""+f.viztag+"\"}");
							val = aaa.result;
							// html += "<td width=10></td><td align=right>"
							html += "<td align=right>"
								+api_sans_set+"&nbsp;"+argname+"&nbsp;=&nbsp;</td><td><input type=\"text\" onChange=\"changeTextVal('"+tag+"','"+api+"','"+argname+"')\" value=\""+val+"\" id=\""+inputid+"\" size=20></td>";
							if ( argname == "paramfile" ) {
								var paramsclass = params_class_of_api(api)
								html += "<td><button onclick=\"edit_"+paramsclass+"_params('"+inputid+"');\" >Edit</button>";
							}

						} else {
							html += "<td><input type=\"button\" style=\"width:150px\" value=\""
								+api+"\" onClick=\"vizapi('"+fullapiname+"','{}');\"></td>";

							// a non-set_* api
							// html += "<td width=10></td><td align=right>"
							// 	+argname+"&nbsp;=&nbsp;</td><td><input type=\"text\" value=\""+val+"\" id=\""+inputid+"\" size=8></td>";
						}

					}
					html += "</tr>";
				}
				html += "</table>";
			}
		}

		html += "<table>";   // for all the parameters
		var api = vizapi("ffff.ffglparamvals","{\"viztag\":\""+f.viztag+"\"}");
		if ( ! checkapi(api) ) {
			continue;
		}
		var vals = api.result;

		if ( vals.length > 0 ) {
			html += "<tr><td colspan=10>FFGL Parameters:</td></tr>";
		}
		for ( var n=0; n<vals.length; n++ ) {
			var v = vals[n];
			// html += "<p>"+v.name+" "+v.value+"\n";
			html += "<tr><td width=15%></td>";

			var name = f.viztag+"."+v.name;
			var valueid = "value_"+name;
			var rangeid = "range_"+name;

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

	document.getElementById("ffglpipeline").innerHTML = html;
	refreshpipelinefilename();
}
