#* This file is part of the MOOSE framework
#* https://www.mooseframework.org
#*
#* All rights reserved, see COPYRIGHT for full restrictions
#* https://github.com/idaholab/moose/blob/master/COPYRIGHT
#*
#* Licensed under LGPL 2.1, please see LICENSE for details
#* https://www.gnu.org/licenses/lgpl-2.1.html

from PyQt5.QtWidgets import QWidget, QTextBrowser
from peacock.utils import WidgetUtils
from peacock.utils import ExeLauncher, TerminalUtils
from PyQt5.QtCore import pyqtSignal
from peacock.base.MooseWidget import MooseWidget
import os

class CheckInputWidget(QWidget, MooseWidget):
    """
    Runs the executable with "--check-input" on the input file and stores the results.
    Signals:
        needInputFile: Emitted when we need the input file. Argument is the path where the input file will be written.
    """
    needInputFile = pyqtSignal(str)

    def __init__(self, **kwds):
        super(CheckInputWidget, self).__init__(**kwds)

        self.input_file = "peacock_check_input.i"
        self.top_layout = WidgetUtils.addLayout(vertical=True)
        self.setLayout(self.top_layout)
        self.output = QTextBrowser(self)
        self.output.setStyleSheet("QTextBrowser { background: black; color: white; }")
        self.output.setReadOnly(True)
        self.top_layout.addWidget(self.output)
        self.button_layout = WidgetUtils.addLayout()
        self.top_layout.addLayout(self.button_layout)
        self.hide_button = WidgetUtils.addButton(self.button_layout, self, "Hide", lambda: self.hide())
        self.check_button = WidgetUtils.addButton(self.button_layout, self, "Check", self._check)
        self.resize(800, 500)
        self.setup()
        self.path = None

    def cleanup(self):
        try:
            os.remove(self.input_file)
        except:
            pass

    def check(self, path):
        """
        Runs the executable with "--check-input" and adds the output to the window
        Input:
            path[str]: Path to the executable
        """
        self.path = path
        self._check()
        self.problems_with_input_file()

    def _check(self):
        """
        Runs the executable with "--check-input" and adds the output to the window
        """
        input_file = os.path.abspath(self.input_file)
        self.needInputFile.emit(input_file)
        self.output.clear()
        try:
            args = ["-i", input_file, "--check-input"]
            output = ExeLauncher.runExe(self.path, args, print_errors=False)
            output_html = TerminalUtils.terminalOutputToHtml(output)
            self.output.setHtml("<pre>%s</pre>" % output_html)
        except Exception as e:
            output_html = TerminalUtils.terminalOutputToHtml(str(e))
            self.output.setHtml("<pre>%s</pre>" % output_html)
        self.cleanup()

    def problems_with_input_file(self):
        data = str(self.output.toPlainText())
        data = data.splitlines()
        errors = []
        while data:
            error = {}
            header = data.pop(0)
            if not header == '*** ERROR ***':
                continue
            prefix = 'Range check failed for parameter '
            if not data[0].startswith(prefix):
                continue
            param_name = data.pop(0)
            param_name = param_name.replace(prefix, '')
            prefix = '\tExpression: '
            if not data[0].startswith(prefix):
                continue
            check_expression = data.pop(0)
            check_expression = check_expression.replace(prefix, '')
            error['param_name'] = param_name
            error['expression'] = check_expression
            errors.append(error)
        print(errors)

    def warnings(self):
        pass
