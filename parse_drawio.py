import xmltodict
import json


def readtxt(fname):
    with open(fname) as f:
        return f.read()


xml = readtxt("src/workflow.drawio")
d = xmltodict.parse(xml)

e = d["mxfile"]["diagram"]["mxGraphModel"]["root"]["mxCell"]


# Organized by ID
nodes = {}


class Node():
    def __init__(self, content):
        self.id = content["@id"]
        self.parent_id = content.get("@parent", None)
        if self.parent_id is not None:
            nodes[self.parent_id].add_child(self)
        self.data = content
        self.name = content.get("@value", None)
        self.geom = None
        if "mxGeometry" in content:
            geom = content["mxGeometry"]
            if "@width" in geom:
                x = int(geom.get("@x", 0))
                y = int(geom.get("@y", 0))
                w = int(geom["@width"])
                h = int(geom["@height"])
                self.geom = (x, y, w, h)
        self.children = []

    def add_child(self, child):
        self.children.append(child)

    @property
    def parent(self):
        if self.parent_id is None:
            return None
        return nodes[self.parent_id]


    def __str__(self):
        name = self.name or "?"
        return f"<Node {name} {self.geom}>"

root = None

windows = {}

for ee in e:
    node = Node(ee)
    if node.parent is None:
        root = node
    nodes[node.id] = node
    if node.name and node.name.startswith('win_'):
        name = node.name[4:]
        windows[name] = node
    # print(node)

def pp(node, pref=''):
    for c in node.children:
        pp(c, pref + ' ')

# pp('', root)
# pp(windows['level'])


lines = []
lines.append('#ifndef CA_LAYOUT_H')
lines.append('#define CA_LAYOUT_H')
def export(win_node, name):
    # print(f'#define WIN_{name}_MODAL ((Rectangle) {{0, 0, {w}, {h}}}) ')
    for node in win_node.children:
        widget_name = node.name
        if node.geom is not None:
            x,y,w,h = node.geom
            lines.append(f'#define WIN_{name}_{widget_name} ((Rectangle) {{{x}, {y}, {w}, {h}}}) ')


export(windows['level'].parent, 'level'.upper())
export(windows['stamp'].parent, 'stamp'.upper())
export(windows['text'].parent, 'text'.upper())

lines.append('#endif')


with open('src/layout.h', 'w') as f:
    f.write('\n'.join(lines))
#    if "@value" not in ee:
#        continue
#    if "mxGeometry" not in ee:
#        continue
#    name = ee["@value"]
#    geom = ee["mxGeometry"]
#    if "@x" not in geom:
#        continue
#    x = geom["@x"]
#    y = geom["@y"]
#    w = geom["@width"]
#    h = geom["@height"]
#    print(name, x, y, w, h)
