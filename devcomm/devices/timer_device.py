"""
Timer device wrapper for the DevCommV3 framework.

This module imports the TimerDevice implementation from the timer output directory.
"""

import sys
import os

# Add path to access the timer device implementation
timer_path = os.path.join(os.path.dirname(__file__), '..', '..', 'timer', 'output')
sys.path.insert(0, timer_path)

from timer_device import TimerDevice, TimerMode, TimerChannel

__all__ = ['TimerDevice', 'TimerMode', 'TimerChannel']