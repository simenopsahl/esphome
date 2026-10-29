import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_CHANNEL, CONF_FREQUENCY, CONF_ID

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1PWMOutput = m5pm1_ns.class_("M5PM1PWMOutput", output.FloatOutput, cg.Component)

CONFIG_SCHEMA = (
    output.FLOAT_OUTPUT_SCHEMA.extend(
        {
            cv.Required(CONF_ID): cv.declare_id(M5PM1PWMOutput),
            cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component),
            cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=1),
            cv.Optional(CONF_FREQUENCY, default="1000Hz"): cv.All(
                cv.frequency, cv.Range(min=1, max=65535)
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await output.register_output(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(var.set_frequency(int(config[CONF_FREQUENCY])))
