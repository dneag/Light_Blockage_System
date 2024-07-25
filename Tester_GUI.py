
import maya.cmds as cmds
import maya.OpenMayaUI as omui

class GUI:

    def __init__(self):

        self.windowName = "light_blockage_tester_window"
        self.guiWidth = 413
        self.createGridFlagsDefault = "un=.4, xs=13., ys=20., zs=13., b=[0,-1.,0],r=2.5,hca=.85,i=2"
        self.clearExistingGUI()

        self.mainWindow_uiID = cmds.window(self.windowName, title="Light Blockage Tester", w=self.guiWidth)

        cmds.scrollLayout(w=self.guiWidth)

        self.makeUI_GridManager()

        cmds.window(self.mainWindow_uiID, edit=True, widthHeight=(self.guiWidth, 320) )
        cmds.showWindow(self.mainWindow_uiID)

    def makeUI_GridManager(self, *_):
        
        cmds.separator()
        cmds.columnLayout(w=self.guiWidth, rs=10)
        cmds.rowColumnLayout(nc=2,cw=[(1,90),(2,315)])
        cmds.text("command flags ")
        self.gridFlagsField = cmds.textField(tx=self.createGridFlagsDefault)
        cmds.setParent("..")
        cmds.rowColumnLayout(nc=2,cw=[(1,200),(2,200)], cs=[(1,0),(2,5)])
        cmds.button(l="Create Grid", command=self.callCreateGrid)
        cmds.button(l="Default flags", command=self.setFlagsToDefault)
        cmds.setParent("..")
        cmds.separator(w=self.guiWidth)
        cmds.text("Display", fn="boldLabelFont", al="center",w=self.guiWidth)
        cmds.rowColumnLayout(w=self.guiWidth, nc=2, cs=[(1,50),(2,10)], rs=[(1,1),(2,15)])
        self.shadedUnitsDisplayCheckBox = cmds.checkBox(l="Show affected boxes",v=0,cc=self.callUpdateGridDisplay)
        self.shadedUnitArrowDisplayCheckBox = cmds.checkBox(l="Show affected arrows",v=1,cc=self.callUpdateGridDisplay)
        cmds.text("Display % Threshhold:   >=")
        self.displayPercntgThreshld = cmds.floatField(v=.01,min=0.,max=.98,s=.01,pre=3,w=60)
        self.deleteBPs = False
         # Note that checkBox's will automatically pass their value to their cc function.  To avoid this, use a lambda
        cmds.button(l="Delete BPs",command=lambda _: self.triggerDeleteBPs())
        cmds.setParent("..")
        cmds.separator(w=self.guiWidth)
        cmds.text("Create Block Points", fn="boldLabelFont", al="center",w=self.guiWidth)
        cmds.rowColumnLayout(w=self.guiWidth, nc=3,cw=[(1,60),(2,35),(3,100)], cs=[(1,20),(2,5),(3,25)])
        cmds.text("Radius")
        self.bpRadiusFld = cmds.floatField(v=.15,min=0.01,max=100.,s=.01,pre=2)
        cmds.button(l="Create BP",command=self.callModifyBlockPoints)

    def callCreateGrid(self, *_):

        flagString = cmds.textField(self.gridFlagsField, q=True, tx=True)
        fullCommand = "cmds.createBlockPointGrid(" + flagString + ")"
        exec(fullCommand)

        self.callUpdateGridDisplay()

    def setFlagsToDefault(self, *_):

        cmds.textField(self.gridFlagsField, e=True,tx=self.createGridFlagsDefault)

    def callModifyBlockPoints(self, *_):

        selections = cmds.ls(selection=True)
        bpLocs = []

        for sel in selections:

            if (".vtx[" in sel):  # selection is a vertex(s)

                if ':' in sel:

                    pre = sel.split('[')[0]
                    indicesString = sel.split('[')[1].split(']')[0].split(':')
                    startIndex = int(indicesString[0].strip())
                    endIndex = int(indicesString[1].strip())

                    for i in range(startIndex, endIndex + 1):

                        bpLocs.extend(cmds.pointPosition(pre + '[' + str(i) + ']', world=True))
                else:  

                    bpLocs.extend(cmds.pointPosition(sel, world=True))

            else: # selection is an object

                location = cmds.xform(sel, query=True, worldSpace=True, translation=True)
                bpLocs.extend(location)

        radius = cmds.floatField(self.bpRadiusFld, q=True, v=True)

        cmds.modifyBlockPoints(c=True, l=bpLocs, den=1, rad=radius)

    def triggerDeleteBPs(self, *_):

        self.deleteBPs = True
        try:
            self.callUpdateGridDisplay()
        except:
            self.deleteBPs = False # set this back to false even when updateGridDisplay fails

        self.deleteBPs = False

    def callUpdateGridDisplay(self, *_):

        displayShadedUnits = cmds.checkBox(self.shadedUnitsDisplayCheckBox, q=True, v=True)
        displayShadedUnitArrows = cmds.checkBox(self.shadedUnitArrowDisplayCheckBox, q=True, v=True)
        displayPercentThresh = cmds.floatField(self.displayPercntgThreshld, q=True, v=True)

        cmds.updateGridDisplay(d=False, dis=1, r=1, nc=2, dbp=True,
                               mtn=False, dlb=self.deleteBPs, dsu=displayShadedUnits, dua=displayShadedUnitArrows,
                               dpt=displayPercentThresh)


    def clearExistingGUI(self, *_):

        if(cmds.window(self.windowName, exists=True)):
            cmds.deleteUI(self.windowName)