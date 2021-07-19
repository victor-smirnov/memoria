import codegen
from jinja2 import Template


class CtrInitGenerator:
    def __init__(self, generator):
        self.generator = generator
        self.template = Template("""
{% for line in global_includes -%}
#include <{{line}}>
{% endfor %}

{% for line in ctr_includes -%}
#include <{{line}}>
{% endfor %}

{% for profile in profiles -%}
{% for incl in profile_includes[profile] -%}
#include <{{incl}}>
{% endfor %}
{% endfor %}


namespace memoria {

{% for dt_init in ctr_datatype_init_decl -%}
    {{dt_init}}
{% endfor %}


void {{function_name}}() {
{% for dt_init in ctr_datatype_init_def -%}
    {{dt_init}}
{% endfor %}

{% for meta_init in ctr_metadata_init -%}
    {{meta_init}}
{% endfor %}
}

}

""")

    def generate_files(self):
        profile_includes = {}
        for pname in self.generator.project().profiles():
            profile_includes[pname] = self.generator.project().profile_includes(pname)
        data = {
            "profiles": self.generator.project().profiles(),
            "profile_includes": profile_includes,
            "project": self.generator.project,
            "global_includes": self.generator.includes(),
            "ctr_metadata_init": self.generator.snippets("ctr_metadata_init"),
            "ctr_datatype_init_def": self.generator.snippets("ctr_datatype_init_def"),
            "ctr_datatype_init_decl": self.generator.snippets("ctr_datatype_init_decl"),
            "ctr_includes": self.generator.snippets("ctr_includes"),
            "function_name": self.generator.config_string("$/function")
        }
        codegen.write_text_file_if_different(
            self.generator.target_file(), self.template.render(data))
