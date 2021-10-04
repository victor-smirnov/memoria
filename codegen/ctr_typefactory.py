import codegen
from jinja2 import Template


class CtrTypeFactory:
    def __init__(self, type_instance, type_factory, profile):
        self.data_type = str(type_instance.type())
        self.type_instance = type_instance
        self.type_factory = type_factory
        self.project = type_instance.project()
        self.profile = "" if profile is None else profile
        self.target_file = type_instance.target_file(self.profile)
        self.init_gen = self.project.generator(type_instance.config_sdn_path() + "/$/init")
        self.template = Template("""
{% for line in full_includes -%}
#include <{{line}}>
{% endfor %}
{% for line in profile_includes -%}
#include <{{line}}>
{% endfor %}

namespace memoria {
using Profile = {{profile}};
using CtrName = {{type_name}};

MMA_INSTANTIATE_CTR_{{factory_type}}(CtrName, Profile)
}
""")

    def dry_run(self, consumer):
        consumer(self.target_file)
        pass

    def generate_init(self):
        self.init_gen.add_snippet("ctr_datatype_init_decl", "MMA_DEFINE_DEFAULT_DATATYPE_OPS({});".format(self.data_type))
        self.init_gen.add_snippet("ctr_datatype_init_def", "register_notctr_operations<{}>();".format(self.data_type))
        for ii in self.type_instance.includes():
            self.init_gen.add_snippet("ctr_includes", ii, True)

    def generate_files(self):
        self.init_gen.add_snippet("ctr_metadata_init", "InitCtrMetadata<{}, {}>();".format(
            self.data_type,
            self.profile
        ))
        data = {
            "full_includes": self.type_instance.full_includes(),
            "profile_includes": self.project.profile_includes(self.profile),
            "profile": self.profile,
            "type_name": str(self.type_instance.type()),
            "factory_type": self.type_factory.type()
        }
        codegen.write_text_file_if_different(
            self.target_file, self.template.render(data)
        )
        print(self.type_instance.type())
