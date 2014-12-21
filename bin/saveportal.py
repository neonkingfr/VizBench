"""
Tutorial - Multiple objects

This tutorial shows you how to create a site structure through multiple
possibly nested request handler objects.
"""

import cherrypy
from cherrypy import tools
from cherrypy.lib.static import serve_file
from mako.template import Template
from mako.lookup import TemplateLookup

lookup = TemplateLookup(directories=['html'])

def error_page_402(status, message, traceback, version):
    return "Error %s - Well, I'm very sorry but you haven't paid!" % status

cherrypy.config.update({'error_page.402': error_page_402})

class HomePage:
    def index(self):
        return '''
            <p>Hi, this is the home page! Check out the other
            fun stuff on this site:</p>
            
            <ul>
                <li><a href="/joke/">A silly joke</a></li>
                <li><a href="/links/">Useful links</a></li>
            </ul>'''
    index.exposed = True

    def afile(self):
	    path = os.path.join(current_dir, "tmp", "two")
	    return serve_file(path, content_type='text/html')
    afile.exposed = True

class JokePage:
    @cherrypy.expose
    def index(self):
    	try:
		tmpl = lookup.get_template("joke.html")
		return tmpl.render(salutation="Hello", target="World")
	except:
		return "Exception!  This is an error page!?"

    index.exposed = True

class DocPage:
    @cherrypy.expose
    def index(self):
    	try:
		tmpl = lookup.get_template("doc.html")
		return tmpl.render(salutation="Hello", target="World")
	except:
		return "Exception!  This is an error page!?"

    default = tools.staticdir.handler(
		    section='static', root="/tmp", dir='static')
    default.exposed = True
    index.exposed = True

class ApiPage:

	exposed = True;

	@cherrypy.expose
	@cherrypy.tools.json_out()
	@cherrypy.tools.json_in()
	def my_route(self):
		result = {"operation": "request", "result": "success"}

		input_json = cherrypy.request.json
		print "HI FROM MY_ROUTE!"
		print "input_json = ",input_json
	    	return result

	def __call__(self):
		print "HI FROM __call__ !"
		result = {"operation": "request", "result": "success"}
		return result


class LinksPage:
    def __init__(self):
        # Request handler objects can create their own nested request
        # handler objects. Simply create them inside their __init__
        # methods!
        self.extra = ExtraLinksPage()
    
    def index(self):
        # Note the way we link to the extra links page (and back).
        # As you can see, this object doesn't really care about its
        # absolute position in the site tree, since we use relative
        # links exclusively.
        return '''
            <p>Here are some useful links:</p>
            
            <ul>
                <li><a href="http://www.cherrypy.org">The CherryPy Homepage</a></li>
                <li><a href="http://www.python.org">The Python Homepage</a></li>
            </ul>
            
            <p>You can check out some extra useful
            links <a href="./extra/">here</a>.</p>
            
            <p>[<a href="../">Return</a>]</p>
        '''
    index.exposed = True


class ExtraLinksPage:
    def index(self):
        # Note the relative link back to the Links page!
        return '''
            <p>Here are some extra useful links:</p>
            
            <ul>
                <li><a href="http://del.icio.us">del.icio.us</a></li>
                <li><a href="http://www.mornography.de">Hendrik's weblog</a></li>
            </ul>
            
            <p>[<a href="../">Return to links page</a>]</p>'''
    index.exposed = True


# Of course we can also mount request handler objects right here!
root = HomePage()
root.joke = JokePage()
root.api = ApiPage()
root.links = LinksPage()
root.doc = DocPage()

# root.doc = tools.staticdir.handler(
 # 		    section='static', root="/tmp", dir='static')
# root.static = tools.staticdir.handler(
# 		    section='static', root="/tmp", dir='static')
# root.static.exposed = True


# Remember, we don't need to mount ExtraLinksPage here, because
# LinksPage does that itself on initialization. In fact, there is
# no reason why you shouldn't let your root object take care of
# creating all contained request handler objects.


import os.path

conf = {
    'global': {
        'server.socket_host': '0.0.0.0',
        'server.socket_port': 4444,
    },
#    '/': {
#        'request.dispatch': cherrypy.dispatch.MethodDispatcher(),
#    }
}

if __name__ == '__main__':
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # CherryPy always starts with app.root when trying to map request URIs
    # to objects, so we need to mount a request handler root. A request
    # to '/' will be mapped to HelloWorld().index().
    cherrypy.quickstart(root, config=conf)

