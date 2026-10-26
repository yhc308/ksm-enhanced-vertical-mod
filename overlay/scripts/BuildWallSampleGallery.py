from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import sys
p=Path(sys.argv[1]);theme=p.name
font=ImageFont.truetype(r"C:\Windows\Fonts\malgun.ttf",15)
small=ImageFont.truetype(r"C:\Windows\Fonts\malgun.ttf",12)
def gallery(names,labels,out,cols,width=270):
 h=round(width*960/540);rows=(len(names)+cols-1)//cols
 im=Image.new("RGB",(cols*(width+10)+10,rows*(h+42)+40),(17,22,31));d=ImageDraw.Draw(im)
 d.text((10,7),theme+" / night / gauge 0%",font=font,fill="white")
 for i,(name,label) in enumerate(zip(names,labels)):
  x=10+i%cols*(width+10);y=38+i//cols*(h+42)
  d.text((x,y),label,font=small,fill="white");im.paste(Image.open(p/name).convert("RGB").resize((width,h)),(x,y+24))
 im.save(p/out)
vals=[-100,-75,-50,-25,0,25,50,75,100]
gallery([f"knob-{v}.png" for v in vals],[f"노브 {v}% · {v/10:+g}°" for v in vals],"knob-gallery.png",3)
z=[-100,-50,0,50,100]
gallery([f"top-{v}.png" for v in z],[f"상단 줌 {v:+d} · {v*.15:+g}°" for v in z],"top-gallery.png",5,width=216)
gallery([f"bottom-{v}.png" for v in z],[f"하단 줌 {v:+d}" for v in z],"bottom-gallery.png",5,width=216)
names=[f"{group}-{v}.png" for group in ["knob","top","bottom"] for v in [-100,0,100]]
labels=[f"{group} {v:+d}" for group in ["노브 %","상단 줌","하단 줌"] for v in [-100,0,100]]
gallery(names,labels,"overview.png",3)
p.joinpath("샘플목록.md").write_text("# "+theme+" 벽 애니메이션 샘플\n\n실제 엔진 렌더. 540×960, night, 게이지0%, 동일 합성 채보의 고정 프리뷰 시점. 노브 샘플은 표시 기울기를 주입한 것으로 실제 입력/판정을 시뮬레이션하지 않습니다. 상단 줌은 현대 방식 회전(값100=15도), 하단 줌은 원본 이동·배율 계산입니다.\n\n- [전체 비교](overview.png)\n- [노브9단계](knob-gallery.png)\n- [상단 줌5단계](top-gallery.png)\n- [하단 줌5단계](bottom-gallery.png)\n\n0값 이미지는 동일한 중립 캡처를 공유합니다. 각 CSV에는 배경 추적량과 안전 제한 여부를 확인할 수 있는 수치가 있습니다.\n",encoding="utf-8")
print(theme+" galleries complete")

if (p/"combined--100--100.png").exists():
 names=["combined--100--100.png","combined--100-100.png","combined-100--100.png","combined-100-100.png","hud-approach.png"]
 labels=["상단-100 / 하단-100 / 기울기+10°","상단-100 / 하단+100 / 기울기+10°","상단+100 / 하단-100 / 기울기+10°","상단+100 / 하단+100 / 기울기+10°","상단+300 · HUD 접근 / 안전 제한"]
 gallery(names,labels,"combined-gallery.png",3)
 md=p/"샘플목록.md";md.write_text(md.read_text(encoding="utf-8")+"\n- [복합 변화·HUD 접근](combined-gallery.png)\n",encoding="utf-8")
