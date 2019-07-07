{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "BUILD",
            "type": "shell",
            "command": "python $BUILD_SCRIPTS_PATH/build_project.py $PROJECT_PATH",
            "group": {
                "kind": "build",
                "isDefault": true
            }
        }
    ],
    "presentation": {
        "reveal":"always"
    }
}