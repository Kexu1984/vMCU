"""
Timer device implementation for the DevCommV3 framework.

This module provides a Timer device that supports:
- 4 independent 32-bit countdown timers
- Timer link mode for extended timing ranges
- Shared interrupt generation (IRQ #18)
- Global enable/disable control
- Error injection capability for fault simulation
"""

import time
import threading
from enum import Enum
from typing import Dict, Any, Optional, Callable, List

# Import base classes from the framework
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from devcomm.core.base_device import BaseDevice
from devcomm.core.registers import RegisterType
from devcomm.utils.event_constants import DeviceOperation


class TimerState(Enum):
    """Timer states."""
    STOPPED = "stopped"
    RUNNING = "running"
    EXPIRED = "expired"


class TimerChannel:
    """Individual timer channel."""

    def __init__(self, channel_id: int):
        self.channel_id = channel_id
        self.load_value = 0
        self.current_value = 0  # Current countdown value
        self.cval_value = 0     # CVAL register (counts up as timer counts down)
        self.enabled = False    # TEN bit
        self.interrupt_enabled = False  # TIE bit
        self.link_enabled = False       # LINKEN bit
        self.state = TimerState.STOPPED
        self.lock = threading.RLock()
        self.last_update_time = 0
        self.interrupt_pending = False

    def reset(self):
        """Reset timer channel to initial state."""
        with self.lock:
            self.load_value = 0
            self.current_value = 0
            self.cval_value = 0
            self.enabled = False
            self.interrupt_enabled = False
            self.link_enabled = False
            self.state = TimerState.STOPPED
            self.last_update_time = 0
            self.interrupt_pending = False

    def configure(self, load_value: int, enabled: bool, interrupt_enabled: bool, link_enabled: bool):
        """Configure timer channel."""
        with self.lock:
            if enabled and not self.enabled:
                # Timer being enabled - load initial value
                self.load_value = load_value
                self.current_value = load_value
                self.cval_value = 0
                self.last_update_time = time.time()
                self.state = TimerState.RUNNING if load_value > 0 else TimerState.EXPIRED
            elif not enabled:
                # Timer being disabled
                self.state = TimerState.STOPPED

            self.enabled = enabled
            self.interrupt_enabled = interrupt_enabled
            self.link_enabled = link_enabled

            # Update load value even if timer is running (takes effect on next reload)
            if load_value != self.load_value:
                self.load_value = load_value


class TimerDevice(BaseDevice):
    """Timer device implementation."""

    # Register offsets (relative to base address)
    TIMER_MCR_REG = 0x000          # Global control register

    # Timer 0 registers
    TIMER0_LDVAL_REG = 0x100       # Load value
    TIMER0_CVAL_REG = 0x104        # Current value (read-only)
    TIMER0_INIT_REG = 0x108        # Control register

    # Timer 1 registers
    TIMER1_LDVAL_REG = 0x110
    TIMER1_CVAL_REG = 0x114
    TIMER1_INIT_REG = 0x118

    # Timer 2 registers
    TIMER2_LDVAL_REG = 0x120
    TIMER2_CVAL_REG = 0x124
    TIMER2_INIT_REG = 0x128

    # Timer 3 registers
    TIMER3_LDVAL_REG = 0x130
    TIMER3_CVAL_REG = 0x134
    TIMER3_INIT_REG = 0x138

    # Timer interrupt number
    TIMER_IRQ = 18

    def __init__(self, name: str, base_address: int, size: int, master_id: int):
        # Initialize timer channels
        self.channels: Dict[int, TimerChannel] = {}
        for i in range(4):
            self.channels[i] = TimerChannel(i)

        # Global state
        self.global_enabled = True  # TIMER_MCR[1] MDIS bit (inverted logic)
        self.timer_thread = None
        self.timer_thread_stop_event = threading.Event()

        # Error injection support
        self.error_injection_enabled = False
        self.injected_errors = {}

        super().__init__(name, base_address, size, master_id)

    def init(self) -> None:
        """Initialize timer registers and state."""
        # Global control register
        self.register_manager.define_register(
            self.TIMER_MCR_REG, "TIMER_MCR", RegisterType.READ_WRITE, 0x00000002,
            write_callback=self._mcr_write_callback,
            read_callback=self._mcr_read_callback
        )

        # Timer 0 registers
        self.register_manager.define_register(
            self.TIMER0_LDVAL_REG, "TIMER0_LDVAL", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._ldval_write_callback(dev, offset, value, 0)
        )
        self.register_manager.define_register(
            self.TIMER0_CVAL_REG, "TIMER0_CVAL", RegisterType.READ_ONLY, 0x00000000,
            read_callback=lambda dev, offset, width: self._cval_read_callback(dev, offset, width, 0)
        )
        self.register_manager.define_register(
            self.TIMER0_INIT_REG, "TIMER0_INIT", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._init_write_callback(dev, offset, value, 0)
        )

        # Timer 1 registers
        self.register_manager.define_register(
            self.TIMER1_LDVAL_REG, "TIMER1_LDVAL", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._ldval_write_callback(dev, offset, value, 1)
        )
        self.register_manager.define_register(
            self.TIMER1_CVAL_REG, "TIMER1_CVAL", RegisterType.READ_ONLY, 0x00000000,
            read_callback=lambda dev, offset, width: self._cval_read_callback(dev, offset, width, 1)
        )
        self.register_manager.define_register(
            self.TIMER1_INIT_REG, "TIMER1_INIT", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._init_write_callback(dev, offset, value, 1)
        )

        # Timer 2 registers
        self.register_manager.define_register(
            self.TIMER2_LDVAL_REG, "TIMER2_LDVAL", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._ldval_write_callback(dev, offset, value, 2)
        )
        self.register_manager.define_register(
            self.TIMER2_CVAL_REG, "TIMER2_CVAL", RegisterType.READ_ONLY, 0x00000000,
            read_callback=lambda dev, offset, width: self._cval_read_callback(dev, offset, width, 2)
        )
        self.register_manager.define_register(
            self.TIMER2_INIT_REG, "TIMER2_INIT", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._init_write_callback(dev, offset, value, 2)
        )

        # Timer 3 registers
        self.register_manager.define_register(
            self.TIMER3_LDVAL_REG, "TIMER3_LDVAL", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._ldval_write_callback(dev, offset, value, 3)
        )
        self.register_manager.define_register(
            self.TIMER3_CVAL_REG, "TIMER3_CVAL", RegisterType.READ_ONLY, 0x00000000,
            read_callback=lambda dev, offset, width: self._cval_read_callback(dev, offset, width, 3)
        )
        self.register_manager.define_register(
            self.TIMER3_INIT_REG, "TIMER3_INIT", RegisterType.READ_WRITE, 0x00000000,
            write_callback=lambda dev, offset, value: self._init_write_callback(dev, offset, value, 3)
        )

        # Start timer processing thread
        self._start_timer_thread()

    def _read_implementation(self, offset: int, width: int) -> int:
        """Read from Timer device registers."""
        return self.register_manager.read_register(self, offset, width)

    def _write_implementation(self, offset: int, value: int, width: int) -> None:
        """Write to Timer device registers."""
        self.register_manager.write_register(self, offset, value, width)

    def _mcr_write_callback(self, device, offset: int, value: int) -> None:
        """Handle writes to TIMER_MCR register."""
        # Bit 1: MDIS (Module Disable) - 1=enabled, 0=disabled
        self.global_enabled = bool(value & 0x2)

        self.trace_manager.log_device_event(self.name, "TIMER_MCR", DeviceOperation.WRITE, {
            'global_enabled': self.global_enabled,
            'value': f'0x{value:08X}'
        })

        # If globally disabled, stop all timers
        if not self.global_enabled:
            for channel in self.channels.values():
                with channel.lock:
                    channel.state = TimerState.STOPPED

    def _mcr_read_callback(self, device, offset: int, width: int) -> int:
        """Handle reads from TIMER_MCR register."""
        value = 0x2 if self.global_enabled else 0x0
        return value

    def _ldval_write_callback(self, device, offset: int, value: int, channel_id: int) -> None:
        """Handle writes to TIMERx_LDVAL registers."""
        if channel_id in self.channels:
            channel = self.channels[channel_id]
            with channel.lock:
                # Update load value
                old_load_value = channel.load_value
                channel.load_value = value & 0xFFFFFFFF

                self.trace_manager.log_device_event(self.name, f"TIMER{channel_id}", DeviceOperation.WRITE, {
                    'register': 'LDVAL',
                    'old_value': f'0x{old_load_value:08X}',
                    'new_value': f'0x{channel.load_value:08X}'
                })

                # If timer is running, new value takes effect on next reload
                # If timer is stopped, we don't change current_value

    def _cval_read_callback(self, device, offset: int, width: int, channel_id: int) -> int:
        """Handle reads from TIMERx_CVAL registers."""
        if channel_id in self.channels:
            channel = self.channels[channel_id]
            with channel.lock:
                # Update CVAL before reading
                self._update_timer_values(channel_id)
                return channel.cval_value & 0xFFFFFFFF
        return 0

    def _init_write_callback(self, device, offset: int, value: int, channel_id: int) -> None:
        """Handle writes to TIMERx_INIT registers."""
        if channel_id in self.channels:
            channel = self.channels[channel_id]

            # Parse control bits
            timer_enabled = bool(value & 0x1)      # TEN - bit 0
            interrupt_enabled = bool(value & 0x2)  # TIE - bit 1
            link_enabled = bool(value & 0x4)       # LINKEN - bit 2

            # Timer 0 cannot be linked (as per spec)
            if channel_id == 0:
                link_enabled = False

            self.trace_manager.log_device_event(self.name, f"TIMER{channel_id}", DeviceOperation.WRITE, {
                'register': 'INIT',
                'timer_enabled': timer_enabled,
                'interrupt_enabled': interrupt_enabled,
                'link_enabled': link_enabled,
                'value': f'0x{value:08X}'
            })

            # Configure the channel
            channel.configure(channel.load_value, timer_enabled, interrupt_enabled, link_enabled)

    def _update_timer_values(self, channel_id: int) -> None:
        """Update timer values based on elapsed time."""
        if channel_id not in self.channels:
            return

        channel = self.channels[channel_id]

        with channel.lock:
            if not self.global_enabled or not channel.enabled or channel.state != TimerState.RUNNING:
                return

            current_time = time.time()

            if channel.last_update_time == 0:
                channel.last_update_time = current_time
                return

            # Calculate elapsed ticks (assuming 1 tick per millisecond for simulation)
            elapsed_time = current_time - channel.last_update_time
            elapsed_ticks = int(elapsed_time * 1000)  # 1ms per tick

            if elapsed_ticks > 0:
                channel.last_update_time = current_time

                if channel.link_enabled and channel_id > 0:
                    # In link mode, this timer only decrements when linked timer expires
                    # We handle this in the timer thread
                    pass
                else:
                    # Normal countdown mode
                    if channel.current_value > elapsed_ticks:
                        channel.current_value -= elapsed_ticks
                        channel.cval_value += elapsed_ticks
                    else:
                        # Timer expired
                        remaining_ticks = elapsed_ticks - channel.current_value
                        channel.cval_value += channel.current_value
                        channel.current_value = 0
                        self._handle_timer_expiry(channel_id)

                        # Handle any remaining ticks after reload
                        if remaining_ticks > 0 and channel.current_value > 0:
                            if channel.current_value > remaining_ticks:
                                channel.current_value -= remaining_ticks
                                channel.cval_value += remaining_ticks
                            else:
                                channel.cval_value += channel.current_value
                                channel.current_value = 0
                                self._handle_timer_expiry(channel_id)

    def _handle_timer_expiry(self, channel_id: int) -> None:
        """Handle timer expiry - reload and generate interrupt/pulse."""
        if channel_id not in self.channels:
            return

        channel = self.channels[channel_id]

        self.trace_manager.log_device_event(self.name, f"TIMER{channel_id}", DeviceOperation.IRQ_TRIGGER, {
            'event': 'timer_expired',
            'load_value': f'0x{channel.load_value:08X}'
        })

        # Reload timer
        channel.current_value = channel.load_value
        channel.cval_value = 0  # Reset CVAL on reload

        # Generate interrupt if enabled
        if channel.interrupt_enabled:
            channel.interrupt_pending = True
            self._trigger_timer_interrupt()

        # Generate pulse for linked timers
        self._generate_link_pulse(channel_id)

        # If load value is 0, timer stops
        if channel.load_value == 0:
            channel.state = TimerState.EXPIRED
        else:
            channel.state = TimerState.RUNNING

    def _generate_link_pulse(self, source_channel_id: int) -> None:
        """Generate pulse for linked timers."""
        # Find timers that are linked to this timer
        for channel_id, channel in self.channels.items():
            if (channel.link_enabled and
                channel_id == source_channel_id + 1 and  # Link to next higher numbered timer
                channel.enabled and
                self.global_enabled):

                with channel.lock:
                    if channel.current_value > 0:
                        channel.current_value -= 1
                        channel.cval_value += 1

                        if channel.current_value == 0:
                            self._handle_timer_expiry(channel_id)

    def _trigger_timer_interrupt(self) -> None:
        """Trigger timer interrupt."""
        if self.bus:
            try:
                self.bus.trigger_interrupt(self.TIMER_IRQ)
                self.trace_manager.log_device_event(self.name, "INTERRUPT", DeviceOperation.TRIGGER, {
                    'irq_number': self.TIMER_IRQ
                })
            except Exception as e:
                self.trace_manager.log_device_event(self.name, "INTERRUPT", DeviceOperation.ERROR, {
                    'error': str(e)
                })

        # Clear interrupt pending flags
        for channel in self.channels.values():
            channel.interrupt_pending = False

    def _start_timer_thread(self) -> None:
        """Start background timer processing thread."""
        if self.timer_thread is None or not self.timer_thread.is_alive():
            self.timer_thread_stop_event.clear()
            self.timer_thread = threading.Thread(target=self._timer_thread_worker, daemon=True)
            self.timer_thread.start()

    def _timer_thread_worker(self) -> None:
        """Background thread for timer processing."""
        while not self.timer_thread_stop_event.wait(0.010):  # 10ms intervals
            try:
                # Update all active timers
                for channel_id in range(4):
                    self._update_timer_values(channel_id)

                # Check for any pending interrupts
                any_interrupt_pending = any(ch.interrupt_pending for ch in self.channels.values())
                if any_interrupt_pending:
                    self._trigger_timer_interrupt()

            except Exception as e:
                # Log error but continue running
                self.trace_manager.log_device_event(self.name, "THREAD", DeviceOperation.IRQ_TRIGGER_FAILED, {
                    'error': str(e)
                })

    def reset(self) -> None:
        """Reset device to initial state."""
        super().reset()

        # Reset all timer channels
        for channel in self.channels.values():
            channel.reset()

        self.global_enabled = True

        # Restart timer thread if needed
        self._start_timer_thread()

    def disable(self) -> None:
        """Disable device operations."""
        super().disable()

        # Stop timer processing
        if self.timer_thread and self.timer_thread.is_alive():
            self.timer_thread_stop_event.set()
            self.timer_thread.join(timeout=1.0)

    def inject_error(self, error_type: str, error_params: dict) -> None:
        """
        Inject errors for fault simulation.

        Args:
            error_type: Type of error ("timer_stuck", "interrupt_fail", "register_corrupt", etc.)
            error_params: Error-specific parameters including channel_id
        """
        channel_id = error_params.get("channel_id", 0)

        # Log error injection
        self.trace_manager.log_device_event(self.name, f"timer_{channel_id}", DeviceOperation.DISABLE, {
            'operation': 'error_injection',
            'error_type': error_type,
            'error_params': error_params
        })

        if error_type == "timer_stuck":
            self._inject_timer_stuck_error(channel_id, error_params)
        elif error_type == "interrupt_fail":
            self._inject_interrupt_fail_error(channel_id, error_params)
        elif error_type == "register_corrupt":
            self._inject_register_corrupt_error(channel_id, error_params)
        elif error_type == "clock_fail":
            self._inject_clock_fail_error(channel_id, error_params)
        else:
            raise ValueError(f"Unsupported error type: {error_type}")

    def _inject_timer_stuck_error(self, channel_id: int, error_params: dict) -> None:
        """Inject timer stuck error - timer stops counting."""
        if channel_id in self.channels:
            channel = self.channels[channel_id]
            with channel.lock:
                channel.state = TimerState.STOPPED
                self.injected_errors[f"timer_stuck_{channel_id}"] = True

    def _inject_interrupt_fail_error(self, channel_id: int, error_params: dict) -> None:
        """Inject interrupt failure error."""
        if channel_id in self.channels:
            channel = self.channels[channel_id]
            with channel.lock:
                channel.interrupt_enabled = False  # Force disable interrupts
                self.injected_errors[f"interrupt_fail_{channel_id}"] = True

    def _inject_register_corrupt_error(self, channel_id: int, error_params: dict) -> None:
        """Inject register corruption error."""
        if channel_id in self.channels:
            channel = self.channels[channel_id]
            with channel.lock:
                # Corrupt load value
                channel.load_value = 0xDEADBEEF
                self.injected_errors[f"register_corrupt_{channel_id}"] = True

    def _inject_clock_fail_error(self, channel_id: int, error_params: dict) -> None:
        """Inject clock failure error - timer runs at wrong speed."""
        # This would affect the timing calculations in _update_timer_values
        self.injected_errors[f"clock_fail_{channel_id}"] = True

    def get_timer_status(self, channel_id: int) -> Dict[str, Any]:
        """Get status information for a timer channel."""
        if channel_id not in self.channels:
            return {}

        channel = self.channels[channel_id]
        with channel.lock:
            return {
                'channel_id': channel_id,
                'load_value': channel.load_value,
                'current_value': channel.current_value,
                'cval_value': channel.cval_value,
                'enabled': channel.enabled,
                'interrupt_enabled': channel.interrupt_enabled,
                'link_enabled': channel.link_enabled,
                'state': channel.state.value,
                'interrupt_pending': channel.interrupt_pending
            }

    def get_device_status(self) -> Dict[str, Any]:
        """Get overall device status."""
        return {
            'global_enabled': self.global_enabled,
            'timer_thread_running': self.timer_thread and self.timer_thread.is_alive(),
            'channels': {i: self.get_timer_status(i) for i in range(4)},
            'injected_errors': list(self.injected_errors.keys())
        }