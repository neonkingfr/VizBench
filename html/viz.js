function format_error(meth,err) {
	var estr = "" + err;
	if ( estr.match(/xmlhttprequest exception 101/i) ) {
		return "Unable to connect to MMTT - is it running?";
	} else {
		return "Method: "+meth+"  Error:" + err;
	}
}

function doapi(meth,params) {

	// The value of params should include the curly braces
	params = (typeof params === "undefined") ? "{ }" : params;

	var id = "12345";
	var url = "/api";

	var postData = "{ \"jsonrpc\": \"2.0\", \"method\": \""+meth+"\", \"params\": "+params+", \"id\":\""+id+"\" }\n";

	var def = dojo.xhrPost( {
		url: url,
		handleAs: "json",
		headers: { "Content-Type": "application/json"},
		sync: true,
		postData: "{ \"jsonrpc\": \"2.0\", \"method\": \""+meth+"\", \"params\": "+params+", \"id\":\""+id+"\" }\n"
	});

	var retval = "";
	var errval = "";

	def.then( function(r) {
		if ( r == null ) {
			errval = "Method: "+meth+"  Error! r is null";
		} else if ( r.error ) {
			errval = "Method: "+meth+"  Error! id:" + r.id + " msg="+r.error.message;
		} else {
			retval = r.result;
		}
		},
		function(err) {
			errval = format_error(meth,err);
		}
	);

	return {result: retval, error: errval};
}

function viztitlegen(titlestr) {
	var title = dojo.byId("title");
	title.innerHTML = "<h2>"+titlestr+"</h2>";
	title.innerHTML += "<br><a href=index.html>Vizlets Control</a>";
	title.innerHTML += "<br><a href=vizserver.html>VizServer Control</a>";
	title.innerHTML += "<br><a href=params.html>Params Editor</a>";
}

function vizpagegen(taginclude,tagexclude) {

	var plugins = dojo.byId("vizplugins");

	var viztags = doapi('viztags').result;

	if ( viztags == "" ) {
		plugins.innerHTML = "No viz plugins are present.";
		return;
	}

	var html = "";

	var tags = viztags.split(",");

	var r;
	for ( var i=0; i<tags.length; i++ ) {
		tag = tags[i];

		if ( ! taginclude.exec(tag) ) {
			continue;
		}
		if ( tagexclude.exec(tag) ) {
			continue;
		}
		html += "<p><hr>"+tag+"\n";

		var desc = doapi(tag+'.description').result;
		html += "<p>Description: "+desc + "\n";

		var about = doapi(tag+'.about').result;
		html += "<p>About: "+about + "\n";

		var apis = doapi(tag+'.apis').result;
		if ( apis == "" ) {
			html += "<p>No APIs";
		} else {
			html += "<table>";
			html += "<tr><td>APIs:</td></tr>";

			var a = apis.split(";");
			for ( var n=0; n<a.length; n++ ) {
				html += "<tr><td></td>";
				var w = a[n].split(/[\(\)]/);
				// We expect the length of w to be either 1 (just an api name) or
				// 3 (api name + argument)
				var api = w[0];
				var fullapiname = tag + "." + api;
				if ( w.length <= 1 ) {
					html += "<td><input type=\"button\" style=\"width:100px\" value=\""+a[n]+"\" onClick=\"doapi('"+fullapiname+"');\"></td>";
				} else if ( w.length != 3 ) {
					alert("Hey, too many arguments on api="+w[0]+" length="+w.length+" an="+a[n]);
				} else {
					var args = w[1].split(",");

					var argname = args[0];
					var inputid = tag+"_"+api;
					var argstr = "&quot;" + argname + "&quot; : "
						+ "&quot;'+document.getElementById(&quot;" + inputid + "&quot;).value+'&quot;";

					html += "<td><input type=\"button\" style=\"width:100px\" value=\""
						+api+"\" onClick=\"doapi('"+fullapiname+"','{"+argstr+"}');\"></td>";

					html += "<td width=10></td><td align=right>"
						+argname+"&nbsp;=&nbsp;</td><td><input type=\"text\" id=\""+inputid+"\" size=8></td>";
					if ( argname == "paramfile" ) {
						html += "<td><button onclick=\"editparams('"+inputid+"');\" >Edit Params</button>";
					}
				}
				html += "</tr>";
			}
			html += "</table>";
		}
	}
	plugins.innerHTML = html;

	// After the HTML is all done and enabled, set the values using any apis that start with "set_"
	for ( var i=0; i<tags.length; i++ ) {
		tag = tags[i];

		if ( ! taginclude.exec(tag) ) {
			continue;
		}
		if ( tagexclude.exec(tag) ) {
			continue;
		}

		var apis = doapi(tag+'.apis').result;
		if ( apis != "" ) {
			var a = apis.split(";");
			for ( var n=0; n<a.length; n++ ) {
				html += "<tr><td></td>";
				var w = a[n].split(/[\(\)]/);
				if ( w.length <= 1 ) {
					continue;
				}
				var api = w[0];
				var args = w[1].split(",");
				if ( api.search("set_") != 0 ) {
					continue;
				}
				argname = api.substring(4);
				var getapi = tag + ".get_" + argname;
				var val = doapi(getapi).result;
				var inputid = tag+"_"+api;
				document.getElementById(inputid).value = val;
			}
		}
	}
}
