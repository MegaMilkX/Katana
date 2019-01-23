## Input
  - Remove loadBindings call from init
  - loadBindings should return false if no bindings file is present
  - add saveBindings
  - In editor, if loadBindings() call failed, add default bindings and call saveBindings()