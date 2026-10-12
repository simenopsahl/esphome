"""LVGL Screenshot Component - serves a PNG screenshot of the LVGL display via HTTP.

Vendored from agillis/esphome-modular-lvgl-buttons (MIT, see LICENSE) with local
additions: top_layer compositing and page navigation endpoints (host only).
"""

import esphome.codegen as cg
from esphome.components.lvgl.defines import CONF_LVGL_ID
from esphome.components.lvgl.lvcode import LvglComponent
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT

CODEOWNERS = ["@dcgrove"]
DEPENDENCIES = ["lvgl"]

lvgl_screenshot_ns = cg.esphome_ns.namespace("lvgl_screenshot")
LvglScreenshot = lvgl_screenshot_ns.class_("LvglScreenshot", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LvglScreenshot),
        cv.GenerateID(CONF_LVGL_ID): cv.use_id(LvglComponent),
        cv.Optional(CONF_PORT, default=8080): cv.port,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # do_capture_() uses lv_snapshot_take(); LVGL 9 ships it disabled by default.
    cg.add_build_flag("-DLV_USE_SNAPSHOT=1")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
    lv_component = await cg.get_variable(config[CONF_LVGL_ID])
    cg.add(var.set_lvgl(lv_component))

