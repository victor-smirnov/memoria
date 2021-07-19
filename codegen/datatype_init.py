import codegen
from jinja2 import Template


class DatatypeInitGenerator:
    def __init__(self, generator):
        self.generator = generator
        self.template = Template("""
{% for line in full_includes -%}
#include <{{line}}>
{% endfor %}
""")

    def generate_files(self):
        data = {
            "full_includes": self.generator.includes()
        }
        codegen.write_text_file_if_different(
            self.generator.target_file(), self.template.render(data))
