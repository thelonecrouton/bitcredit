from PyQt4 import QtCore, QtGui
import requests,json,sys


try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_Form(QtGui.QWidget):
    def __init__(self):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)

    def setupUi(self, Form):
        Form.setObjectName(_fromUtf8("Form"))
        Form.resize(427, 338)
        self.gridLayout = QtGui.QGridLayout(Form)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.label = QtGui.QLabel(Form)
        self.label.setObjectName(_fromUtf8("label"))
        self.gridLayout.addWidget(self.label, 10, 0, 1, 1)
        self.issue_amnt = QtGui.QLineEdit(Form)
        self.issue_amnt.setText(_fromUtf8(""))
        self.issue_amnt.setObjectName(_fromUtf8("issue_amnt"))
        self.gridLayout.addWidget(self.issue_amnt, 9, 2, 1, 1)
        self.addressWidget = QtGui.QTreeWidget(Form)
        self.addressWidget.setObjectName(_fromUtf8("addressWidget"))
        self.gridLayout.addWidget(self.addressWidget, 0, 0, 1, 3)
        self.label_2 = QtGui.QLabel(Form)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.gridLayout.addWidget(self.label_2, 7, 0, 1, 1)
        self.snd_add = QtGui.QLineEdit(Form)
        self.snd_add.setText(_fromUtf8(""))
        self.snd_add.setObjectName(_fromUtf8("snd_add"))
        self.gridLayout.addWidget(self.snd_add, 11, 0, 1, 1)
        self.label_3 = QtGui.QLabel(Form)
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.gridLayout.addWidget(self.label_3, 7, 2, 1, 1)
        self.sa_btn = QtGui.QPushButton(Form)
        self.sa_btn.setObjectName(_fromUtf8("sa_btn"))
        self.gridLayout.addWidget(self.sa_btn, 13, 0, 1, 1)
        self.ia_btn = QtGui.QPushButton(Form)
        self.ia_btn.setObjectName(_fromUtf8("ia_btn"))
        self.gridLayout.addWidget(self.ia_btn, 13, 2, 1, 1)
        self.assetsWidget = QtGui.QTreeWidget(Form)
        self.assetsWidget.setObjectName(_fromUtf8("assetsWidget"))
        self.gridLayout.addWidget(self.assetsWidget, 2, 0, 1, 3)
        self.label_4 = QtGui.QLabel(Form)
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.gridLayout.addWidget(self.label_4, 10, 2, 1, 1)
        self.issue_add = QtGui.QLineEdit(Form)
        self.issue_add.setText(_fromUtf8(""))
        self.issue_add.setObjectName(_fromUtf8("issue_add"))
        self.gridLayout.addWidget(self.issue_add, 11, 2, 1, 1)
        self.snd_amnt = QtGui.QLineEdit(Form)
        self.snd_amnt.setText(_fromUtf8(""))
        self.snd_amnt.setObjectName(_fromUtf8("snd_amnt"))
        self.gridLayout.addWidget(self.snd_amnt, 9, 0, 1, 1)
        self.rf_btn = QtGui.QPushButton(Form)
        self.rf_btn.setObjectName(_fromUtf8("rf_btn"))
        self.gridLayout.addWidget(self.rf_btn, 3, 2, 1, 1)
        self.chw_btn = QtGui.QPushButton(Form)
        self.chw_btn.setObjectName(_fromUtf8("chw_btn"))
        self.gridLayout.addWidget(self.chw_btn, 3, 0, 1, 1)

        self.retranslateUi(Form)
        QtCore.QMetaObject.connectSlotsByName(Form)

    def retranslateUi(self, Form):
        Form.setWindowTitle(_translate("Form", "ColorUI", None))
        self.addressWidget.headerItem().setText(0, _translate("Form", "Addresses", None))
        __sortingEnabled = self.addressWidget.isSortingEnabled()
        self.addressWidget.setSortingEnabled(False)
        getBalance()

        for item in r:
            qi = QtGui.QTreeWidgetItem()
            qi.setText(0, _translate("Form", item['oa_address'], None))
            qi_c = QtGui.QTreeWidgetItem()
            qi_c.setText(0, _translate("Form", item['value'], None))
            qi.addChild(qi_c)
            self.addressWidget.addTopLevelItem(qi)

        self.addressWidget.setSortingEnabled(__sortingEnabled)
        self.assetsWidget.headerItem().setText(0, _translate("Form", "Assets", None))
        __sortingEnabled = self.assetsWidget.isSortingEnabled()
        self.assetsWidget.setSortingEnabled(False)

        for item in r:
            for a in item['assets']:
                qi = QtGui.QTreeWidgetItem()
                qi.setText(0, _translate("Form", a['asset_id'], None))
                qi_c = QtGui.QTreeWidgetItem()
                qi_c.setText(0, _translate("Form", a['quantity'], None))
                qi.addChild(qi_c)

                self.assetsWidget.addTopLevelItem(qi)

        self.assetsWidget.setSortingEnabled(__sortingEnabled)
        self.label_2.setText(_translate("Form", "Amount:", None))
        self.label_3.setText(_translate("Form", "Amount:", None))
        self.sa_btn.setText(_translate("Form", "Send Assets", None))
        self.ia_btn.setText(_translate("Form", "Issue Assets", None))
        self.assetsWidget.headerItem().setText(0, _translate("Form", "Assets", None))
        __sortingEnabled = self.assetsWidget.isSortingEnabled()
        self.assetsWidget.setSortingEnabled(False)
        self.assetsWidget.setSortingEnabled(__sortingEnabled)
        self.label_4.setText(_translate("Form", "Address:", None))
        self.label.setText(_translate("Form", "Address:", None))
        self.rf_btn.setText(_translate("Form", "Refresh", None))
        self.chw_btn.setText(_translate("Form", "Change Wallet", None))
        self.rf_btn.clicked.connect(self.rebuild)
        self.chw_btn.clicked.connect(self.chWallet)
        self.assetsWidget.currentItemChanged.connect(self.assetClick)
        self.addressWidget.currentItemChanged.connect(self.addyClick)
        self.sa_btn.clicked.connect(self.sendAsset)
        self.sa_btn.clicked.connect(self.rebuild)
        self.ia_btn.clicked.connect(self.issueAsset)
        self.ia_btn.clicked.connect(self.rebuild)


    def rebuild(form):
        getBalance()
        form.addressWidget.clear()
        for item in r:
            qi = QtGui.QTreeWidgetItem()
            qi.setText(0, _translate("Form", item['oa_address'], None))
            qi_c = QtGui.QTreeWidgetItem()
            qi_c.setText(0, _translate("Form", item['value'], None))
            qi.addChild(qi_c)
            form.addressWidget.addTopLevelItem(qi)

        form.assetsWidget.clear()
        for item in r:
            for a in item['assets']:
                qi = QtGui.QTreeWidgetItem()
                qi.setText(0, _translate("Form", a['asset_id'], None))
                qi_c = QtGui.QTreeWidgetItem()
                qi_c.setText(0, _translate("Form", a['quantity'], None))
                qi.addChild(qi_c)

                form.assetsWidget.addTopLevelItem(qi)

    def sendAsset(form):
        global r2
        payload = {'address': "%s"%addy, 'asset': "%s"%asset, 'amount': "%s"%form.snd_amnt.text(), "to": "%s"%form.snd_add.text()}
        r2 = requests.post(url + "sendasset", data=payload)
        print r2.json(),asset,form.snd_add.text(),form.snd_amnt.text()
        print(r2.url)
        global stuff
        stuff = r2.json()
        ex3 = popupForm()
        ex3.show()

    def issueAsset(form):
        global r3
        payload = {'address': "%s"%addy, "to": "%s"%form.issue_add.text(), 'amount': "%s"%form.issue_amnt.text()}
        r3 = requests.post(url + "issueasset", data=payload)
        print r3.json(),addy,form.snd_add.text(),form.snd_amnt.text()
        print(r3.url)
        #global stuff
        #stuff = r3.json()
        #ex3 = popupForm()
        #ex3.show()

    def chWallet(self):
        ex.hide()
        ex2.show()

    def assetClick(self, current, previous):
        if (previous != None):
            print "old: " + previous.text(0)
        global asset
        asset = current.text(0)

    def addyClick(self, current, previous):
        if (previous != None):
            print "old: " + previous.text(0)
        global addy
        addy = current.text(0)

class Ui_Form2(QtGui.QWidget):
    def __init__(self):
        QtGui.QWidget.__init__(self)
        self.setupUi2(self)
    def setupUi2(self, Form):
        Form.setObjectName(_fromUtf8("Form"))
        Form.resize(528, 227)
        self.label = QtGui.QLabel(Form)
        self.label.setGeometry(QtCore.QRect(190, -10, 141, 101))
        self.label.setObjectName(_fromUtf8("label"))
        self.address = QtGui.QLineEdit(Form)
        self.address.setGeometry(QtCore.QRect(150, 140, 231, 16))
        self.address.setObjectName(_fromUtf8("address"))
        self.label_2 = QtGui.QLabel(Form)
        self.label_2.setGeometry(QtCore.QRect(230, 110, 51, 16))
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.pushButton = QtGui.QPushButton(Form)
        self.pushButton.setGeometry(QtCore.QRect(220, 180, 81, 23))
        self.pushButton.setObjectName(_fromUtf8("pushButton"))
        self.retranslateUi2(Form)
        QtCore.QMetaObject.connectSlotsByName(Form)

    def retranslateUi2(self, Form):
        Form.setWindowTitle(_translate("Form", "ColorUI", None))
        self.label.setText(_translate("Form", "<html><head/><body><p><span style=\" font-size:28pt; font-weight:600;\">ColorUI</span></p></body></html>", None))
        self.label_2.setText(_translate("Form", "Enter URL:", None))
        self.pushButton.setText(_translate("Form", "Access Wallet", None))
        self.pushButton.clicked.connect(self.screen2)

    def screen2(form):
        global url
        url = form.address.text()
        requests.post(url + "getbalance").json()
        global ex
        ex = Ui_Form()
        ex2.hide()
        ex.show()

def getBalance():
    global r
    r = requests.post(url + "getbalance").json()

class popupForm(QtGui.QWidget):
    def __init__(self):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)

    def setupUi(self, Form):
        Form.setObjectName(_fromUtf8("Form"))
        Form.resize(276, 88)
        self.horizontalLayout = QtGui.QHBoxLayout(Form)
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.label = QtGui.QLabel(Form)
        self.label.setObjectName(_fromUtf8("label"))
        self.horizontalLayout.addWidget(self.label)

        self.uiText(Form)
        QtCore.QMetaObject.connectSlotsByName(Form)

    def uiText(self, Form):
        print stuff
        Form.setWindowTitle(_translate("Form", "Result", None))
        self.label.setText(_translate("Form", "<html><head/><body><p><span style=\" font-size:14pt;\">%s</span></p></body></html>"%stuff, None))


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    global ex2
    ex2 = Ui_Form2()
    ex2.show()
    sys.exit(app.exec_())
