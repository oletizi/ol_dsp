# Feature: 360

The goal of this feature is to make it easy to define controller
  parameters in various custom modes in a Novation Launch Control XL3 device
  via the launch-control-xl3 library (probably via an external web interface),
  then use the parameter names + cc numbere from those custom modes to create
  a "canonical mappings" of cc numbers to VST/AU/etc. plugin parameters for
  the plugins that match the launch control custom modes. And THEN translate
  and apply those canonical mappings to various supported Digital Audio
  Workstations (DAWs)--currently Ardour via midi map files and Ableton Live
  via a custom Max For Live patch. That "round trip" capabilities is why the
  feature branch is called 'cc-mapping-360'; If we don't alredy have planning
  and/or implementation documentation, we should write a workplan document to
  docs/1.0
