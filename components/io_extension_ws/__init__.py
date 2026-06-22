# ============================================================================
#  Interfaccia ESPHome (Python) per il componente custom io_extension_ws
#  Espone:
#    - il componente hub (io_extension_ws:)
#    - azioni lambda usabili da automazioni: set_output_pin / set_pwm
#    - uno schema pin (io_extension_ws:) usabile come reset_pin di altri
#      componenti, es. il reset del touch GT911 su IO1
# ============================================================================
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_INVERTED,
    CONF_MODE,
    CONF_NUMBER,
    CONF_OUTPUT,
)

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@francesco"]
MULTI_CONF = True

io_extension_ws_ns = cg.esphome_ns.namespace("io_extension_ws")
IoExtensionWS = io_extension_ws_ns.class_(
    "IoExtensionWS", cg.Component, i2c.I2CDevice
)
IoExtensionWSGPIOPin = io_extension_ws_ns.class_(
    "IoExtensionWSGPIOPin", cg.GPIOPin
)

CONF_IO_EXTENSION_WS = "io_extension_ws"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(IoExtensionWS),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x24))  # indirizzo fisso 0x24
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)


# --- Schema pin: permette `io_extension_ws: <id>` come pin (es. reset_pin) ---
IO_EXTENSION_WS_PIN_SCHEMA = pins.gpio_base_schema(
    IoExtensionWSGPIOPin,
    cv.int_range(min=0, max=7),
).extend(
    {
        cv.Required(CONF_IO_EXTENSION_WS): cv.use_id(IoExtensionWS),
    }
)


@pins.PIN_SCHEMA_REGISTRY.register(CONF_IO_EXTENSION_WS, IO_EXTENSION_WS_PIN_SCHEMA)
async def io_extension_ws_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_IO_EXTENSION_WS])
    cg.add(var.set_parent(parent))
    cg.add(var.set_pin(config[CONF_NUMBER]))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
