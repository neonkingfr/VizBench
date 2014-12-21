import cherrypy

class Api(object):

    def __init__(self, content):
        self.content = content

    exposed = True

    def GET(self):
	return "content=%s" % self.content

    def PUT(self):
        self.content = "PUT INPUT"

    def to_html(self):
	items = "bogus items"
        return '<html>{items}</html>'.format(**vars())

    @staticmethod
    def from_html(data):
        pattern = re.compile(r'\<div\>(?P<name>.*?)\:(?P<value>.*?)\</div\>')
        items = [match.groups() for match in pattern.finditer(data)]
        return dict(items)

class Resource(object):

    def __init__(self, content):
        self.content = content

    exposed = True

    def GET(self):
        c = self.content.items()
	print "================= GET!!  body c=",c
	return self.to_html()

    def PUT(self):
        self.content = self.from_html(cherrypy.request.body.read())

    def to_html(self):
        # html_item = lambda (name,value): '<div>{name}:{value}</div>'.format(\*\*vars())
        # items = map(html_item, self.content.items())
        # items = ''.join(items)
	items = "bogus items"
        return '<html>{items}</html>'.format(**vars())

    @staticmethod
    def from_html(data):
        pattern = re.compile(r'\<div\>(?P<name>.*?)\:(?P<value>.*?)\</div\>')
        items = [match.groups() for match in pattern.finditer(data)]
        return dict(items)

class ResourceIndex(Resource):
    def to_html(self):
        # html_item = lambda (name,value): '<div><a href="{value}">{name}</a></div>'.format(\*\*vars())
        # items = map(html_item, self.content.items())
        # items = ''.join(items)
	items = "more bogus"
        return '<html>{items}</html>'.format(**vars())

class Root(object):
    pass

root = Root()

root.api = Api("dummy content")

root.sidewinder = Resource({'color': 'red', 'weight': 176, 'type': 'stable'})
root.teebird = Resource({'color': 'green', 'weight': 173, 'type': 'overstable'})
root.blowfly = Resource({'color': 'purple', 'weight': 169, 'type': 'putter'})
root.resource_index = ResourceIndex({'sidewinder': 'sidewinder', 'teebird': 'teebird', 'blowfly': 'blowfly'})

conf = {
    'global': {
        'server.socket_host': '0.0.0.0',
        'server.socket_port': 4444,
    },
    '/': {
        'request.dispatch': cherrypy.dispatch.MethodDispatcher(),
    }
}

cherrypy.quickstart(root, '/', conf)

