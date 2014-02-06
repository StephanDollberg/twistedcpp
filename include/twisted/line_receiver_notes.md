# Notes about line_receiver

Current implementation was reset to the most basic implementation

Optimization thoughts:
 - Add Remove unnecessary mem copies
 - split up handling of first line with possible rests and the lines after
 - use basic_protocol buffer directly
