import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_EMPTY,
    UNIT_HERTZ,
    UNIT_SECOND,
    UNIT_VOLT,
)

from . import CONF_M5PM1_ID, M5PM1Component, m5pm1_ns

DEPENDENCIES = ["m5pm1"]

M5PM1Sensor = m5pm1_ns.class_("M5PM1Sensor", sensor.Sensor, cg.PollingComponent)
SensorType = m5pm1_ns.enum("M5PM1SensorType")

SENSOR_TYPES = {
    "battery_voltage": SensorType.SENSOR_BATTERY_VOLTAGE,
    "vin_voltage": SensorType.SENSOR_VIN_VOLTAGE,
    "vinout_voltage": SensorType.SENSOR_VINOUT_VOLTAGE,
    "reference_voltage": SensorType.SENSOR_REFERENCE_VOLTAGE,
    "temperature": SensorType.SENSOR_TEMPERATURE,
    "adc1": SensorType.SENSOR_ADC1,
    "adc2": SensorType.SENSOR_ADC2,
    "watchdog_count": SensorType.SENSOR_WATCHDOG_COUNT,
    "pwm_frequency": SensorType.SENSOR_PWM_FREQUENCY,
}


def _schema(**kwargs):
    return (
        sensor.sensor_schema(M5PM1Sensor, **kwargs)
        .extend({cv.GenerateID(CONF_M5PM1_ID): cv.use_id(M5PM1Component)})
        .extend(cv.polling_component_schema("60s"))
    )


_VOLTAGE = dict(
    unit_of_measurement=UNIT_VOLT,
    accuracy_decimals=3,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
)
_COUNTS = dict(
    unit_of_measurement=UNIT_EMPTY,
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = cv.typed_schema(
    {
        "battery_voltage": _schema(**_VOLTAGE),
        "vin_voltage": _schema(**_VOLTAGE),
        "vinout_voltage": _schema(**_VOLTAGE),
        "reference_voltage": _schema(**_VOLTAGE),
        "temperature": _schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        "adc1": _schema(**_COUNTS),
        "adc2": _schema(**_COUNTS),
        "watchdog_count": _schema(
            unit_of_measurement=UNIT_SECOND, accuracy_decimals=0
        ),
        "pwm_frequency": _schema(
            unit_of_measurement=UNIT_HERTZ, accuracy_decimals=0
        ),
    },
    lower=True,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_M5PM1_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(SENSOR_TYPES[config[CONF_TYPE]]))
