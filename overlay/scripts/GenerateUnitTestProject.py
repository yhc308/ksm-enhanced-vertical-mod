"""Generate a Windows Catch2 target using the game project settings."""
from pathlib import Path
import sys, xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
ns="http://schemas.microsoft.com/developer/msbuild/2003"
ET.register_namespace("",ns)
def tag(name):return "{"+ns+"}"+name
source=root/"kshootmania/kshootmania.vcxproj"
tree=ET.parse(source);project=tree.getroot()
for node in project.iter():
    if "Include" in node.attrib and node.tag in {tag("ClCompile"),tag("ClInclude"),tag("Font"),tag("ProjectReference"),tag("ResourceCompile")}:
        node.set("Include",str((source.parent/node.get("Include").replace("\\","/")).resolve()))
    if node.text:node.text=node.text.replace("$(ProjectDir)",str(source.parent)+"/")
for group in list(project):
    for node in list(group):
        if node.tag==tag("ClCompile") and Path(node.get("Include","")).name=="Main.cpp":group.remove(node)
    if group.tag==tag("PropertyGroup"):
        for node in group:
            if node.tag==tag("ProjectName"):node.text="ksm-v2-tests"
            if node.tag==tag("ProjectGuid"):node.text="{56659494-55DD-4D56-A5D9-4F5701900D42}"
            if node.tag==tag("TargetName"):node.text="ksm-v2-tests"
            if node.tag==tag("KsmEnableDiagnostics"):node.text="1"
for node in project.iter(tag("AdditionalIncludeDirectories")):
    node.text=str(root/"build/deps/include")+";"+(node.text or "")
group=ET.Element(tag("ItemGroup"))
for path in sorted((root/"tests").glob("*.cpp")):ET.SubElement(group,tag("ClCompile"),{"Include":str(path)})
project.insert(len(project)-2,group)
resources=ET.Element(tag("ItemDefinitionGroup"))
compile=ET.SubElement(resources,tag("ResourceCompile"))
ET.SubElement(compile,tag("AdditionalIncludeDirectories")).text=str(source.parent/"App")+";"+str(source.parent)+";%(AdditionalIncludeDirectories)"
project.insert(len(project)-2,resources)
output=root/"build/unit-project/ksm-v2-tests.vcxproj";output.parent.mkdir(parents=True,exist_ok=True)
tree.write(output,encoding="utf-8",xml_declaration=True)
print(output)
