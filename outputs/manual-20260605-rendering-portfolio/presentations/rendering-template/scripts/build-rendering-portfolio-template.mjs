import fs from "node:fs";
import path from "node:path";
import { createRequire } from "node:module";

const require = createRequire("C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/.pnpm/pptxgenjs@4.0.1/node_modules/pptxgenjs/package.json");
const pptxgen = require("pptxgenjs");

const outDir = path.resolve("Portfolio");
const outFile = path.join(outDir, "DirectX11_Rendering_TechDemo_Template.pptx");
fs.mkdirSync(outDir, { recursive: true });

const pptx = new pptxgen();
pptx.layout = "LAYOUT_WIDE";
pptx.author = "Jellyto Studio";
pptx.subject = "DirectX11 rendering engine portfolio section template";
pptx.title = "DirectX11 Rendering Tech Demo";
pptx.company = "Jellyto Studio";
pptx.lang = "ko-KR";
pptx.theme = {
  headFontFace: "Aptos Display",
  bodyFontFace: "Aptos",
  lang: "ko-KR",
};
pptx.defineLayout({ name: "WIDE", width: 13.333, height: 7.5 });
pptx.layout = "WIDE";
pptx.margin = 0;

const C = {
  bg: "F7F8FA",
  ink: "111827",
  muted: "64748B",
  line: "CBD5E1",
  panel: "FFFFFF",
  dark: "0F172A",
  blue: "2563EB",
  cyan: "0891B2",
  green: "16A34A",
  amber: "D97706",
  red: "DC2626",
  softBlue: "DBEAFE",
  softGreen: "DCFCE7",
  softAmber: "FEF3C7",
  softRed: "FEE2E2",
};

const W = 13.333;
const H = 7.5;

function addBg(slide) {
  slide.background = { color: C.bg };
  slide.addShape(pptx.ShapeType.rect, {
    x: 0,
    y: 0,
    w: W,
    h: H,
    fill: { color: C.bg },
    line: { color: C.bg, transparency: 100 },
  });
}

function addTop(slide, section, page) {
  slide.addText(section, {
    x: 0.45,
    y: 0.25,
    w: 6.8,
    h: 0.22,
    fontFace: "Aptos",
    fontSize: 7.5,
    bold: true,
    color: C.blue,
    charSpace: 0.5,
    margin: 0,
  });
  slide.addText(String(page).padStart(2, "0"), {
    x: 12.32,
    y: 0.22,
    w: 0.55,
    h: 0.28,
    fontSize: 8,
    bold: true,
    color: C.muted,
    align: "right",
    margin: 0,
  });
  slide.addShape(pptx.ShapeType.line, {
    x: 0.45,
    y: 0.62,
    w: 12.42,
    h: 0,
    line: { color: "E2E8F0", width: 0.75 },
  });
}

function title(slide, text, subtitle) {
  slide.addText(text, {
    x: 0.45,
    y: 0.8,
    w: 7.5,
    h: 0.55,
    fontFace: "Aptos Display",
    fontSize: 25,
    bold: true,
    color: C.ink,
    breakLine: false,
    margin: 0,
    fit: "shrink",
  });
  if (subtitle) {
    slide.addText(subtitle, {
      x: 0.47,
      y: 1.38,
      w: 7.5,
      h: 0.38,
      fontSize: 10.5,
      color: C.muted,
      margin: 0,
      fit: "shrink",
    });
  }
}

function label(slide, text, x, y, w, color = C.muted) {
  slide.addText(text, {
    x,
    y,
    w,
    h: 0.22,
    fontSize: 7.5,
    bold: true,
    color,
    margin: 0,
    fit: "shrink",
  });
}

function body(slide, lines, x, y, w, h, opts = {}) {
  slide.addText(lines.join("\n"), {
    x,
    y,
    w,
    h,
    fontSize: opts.fontSize ?? 10.2,
    color: opts.color ?? C.ink,
    breakLine: false,
    valign: "top",
    fit: "shrink",
    margin: opts.margin ?? 0.03,
    paraSpaceAfterPt: opts.paraSpaceAfterPt ?? 4,
    bullet: opts.bullet ? { type: "ul" } : undefined,
  });
}

function placeholder(slide, x, y, w, h, caption, hint = "캡처 이미지를 이 영역에 붙여넣기") {
  slide.addShape(pptx.ShapeType.rect, {
    x,
    y,
    w,
    h,
    fill: { color: "F8FAFC" },
    line: { color: "94A3B8", width: 1.2, dash: "dash" },
  });
  slide.addText(caption, {
    x: x + 0.18,
    y: y + h / 2 - 0.18,
    w: w - 0.36,
    h: 0.28,
    fontSize: 11,
    bold: true,
    color: C.ink,
    align: "center",
    margin: 0,
    fit: "shrink",
  });
  slide.addText(hint, {
    x: x + 0.18,
    y: y + h / 2 + 0.12,
    w: w - 0.36,
    h: 0.25,
    fontSize: 8,
    color: C.muted,
    align: "center",
    margin: 0,
    fit: "shrink",
  });
}

function pill(slide, text, x, y, w, fill, color = C.ink) {
  slide.addShape(pptx.ShapeType.roundRect, {
    x,
    y,
    w,
    h: 0.36,
    rectRadius: 0.06,
    fill: { color: fill },
    line: { color: fill },
  });
  slide.addText(text, {
    x: x + 0.08,
    y: y + 0.09,
    w: w - 0.16,
    h: 0.16,
    fontSize: 7.5,
    bold: true,
    color,
    align: "center",
    margin: 0,
    fit: "shrink",
  });
}

function metric(slide, labelText, value, x, y, w, accent) {
  slide.addShape(pptx.ShapeType.rect, {
    x,
    y,
    w,
    h: 0.82,
    fill: { color: C.panel },
    line: { color: "E2E8F0", width: 0.6 },
  });
  slide.addShape(pptx.ShapeType.rect, {
    x,
    y,
    w: 0.06,
    h: 0.82,
    fill: { color: accent },
    line: { color: accent },
  });
  slide.addText(value, {
    x: x + 0.18,
    y: y + 0.13,
    w: w - 0.24,
    h: 0.26,
    fontSize: 15,
    bold: true,
    color: C.ink,
    margin: 0,
    fit: "shrink",
  });
  slide.addText(labelText, {
    x: x + 0.18,
    y: y + 0.47,
    w: w - 0.24,
    h: 0.17,
    fontSize: 7.3,
    color: C.muted,
    margin: 0,
    fit: "shrink",
  });
}

function flowNode(slide, text, x, y, w, accent, fill = C.panel) {
  slide.addShape(pptx.ShapeType.rect, {
    x,
    y,
    w,
    h: 0.68,
    fill: { color: fill },
    line: { color: accent, width: 1.1 },
  });
  slide.addText(text, {
    x: x + 0.08,
    y: y + 0.23,
    w: w - 0.16,
    h: 0.2,
    fontSize: 8.8,
    bold: true,
    color: C.ink,
    align: "center",
    margin: 0,
    fit: "shrink",
  });
}

function arrow(slide, x, y, w) {
  slide.addShape(pptx.ShapeType.line, {
    x,
    y,
    w,
    h: 0,
    line: { color: "94A3B8", width: 1.1, beginArrowType: "none", endArrowType: "triangle" },
  });
}

function table(slide, rows, x, y, colW, rowH = 0.38) {
  rows.forEach((row, r) => {
    row.forEach((cell, c) => {
      const isHead = r === 0;
      slide.addShape(pptx.ShapeType.rect, {
        x: x + colW.slice(0, c).reduce((a, b) => a + b, 0),
        y: y + r * rowH,
        w: colW[c],
        h: rowH,
        fill: { color: isHead ? C.dark : r % 2 ? "FFFFFF" : "F8FAFC" },
        line: { color: "CBD5E1", width: 0.35 },
      });
      slide.addText(cell, {
        x: x + colW.slice(0, c).reduce((a, b) => a + b, 0) + 0.06,
        y: y + r * rowH + 0.095,
        w: colW[c] - 0.12,
        h: rowH - 0.14,
        fontSize: isHead ? 7.8 : 7.4,
        bold: isHead,
        color: isHead ? "FFFFFF" : C.ink,
        margin: 0,
        fit: "shrink",
      });
    });
  });
}

function slide01() {
  const slide = pptx.addSlide();
  addBg(slide);
  addTop(slide, "DIRECTX11 RENDERING ENGINE TECH DEMO", 1);
  title(
    slide,
    "DirectX11 대량 블록 렌더링 최적화 테크 데모",
    "상용 게임 완성이 아니라, 저수준 렌더링 제어와 병목 분석 능력을 증명하기 위한 엔진 포트폴리오"
  );
  pill(slide, "C++ / DirectX11", 0.48, 1.94, 1.35, C.softBlue, C.blue);
  pill(slide, "Hardware Instancing", 1.95, 1.94, 1.65, C.softGreen, C.green);
  pill(slide, "Controlled Benchmark", 3.72, 1.94, 1.74, C.softAmber, C.amber);
  body(slide, [
    "목표: 수천~수만 개 블록이 동시에 존재하는 장면에서 DrawCall, 렌더 후보, 재빌드 비용을 제어",
    "범위: 게임성보다 렌더링 파이프라인, Chunk 관리, RHI 분리, 검증 도구 구축에 집중",
    "결과: 최적화 ON/OFF를 같은 씬에서 비교 가능한 Stress Benchmark HUD 구현",
  ], 0.52, 2.48, 5.05, 1.38, { bullet: true, fontSize: 10.2 });
  metric(slide, "개발 기간", "3개월", 0.52, 4.34, 1.6, C.blue);
  metric(slide, "검증 방식", "ON/OFF", 2.28, 4.34, 1.6, C.green);
  metric(slide, "주요 병목", "CPU-GPU", 4.04, 4.34, 1.6, C.amber);
  placeholder(slide, 6.05, 1.04, 6.82, 5.78, "FINAL HERO SCREENSHOT / GIF", "Stress HUD + 대량 블록 화면 캡처");
  body(slide, [
    "붙여넣을 이미지: 최종 실행 화면 1장",
    "권장: Stress HUD가 보이고 블록 밀도가 높은 구도",
  ], 6.16, 6.95, 6.6, 0.28, { fontSize: 7.6, color: C.muted });
}

function slide02() {
  const slide = pptx.addSlide();
  addBg(slide);
  addTop(slide, "ARCHITECTURE", 2);
  title(slide, "게임 로직과 렌더링 하드웨어 인터페이스를 분리", "Scene 데이터는 RenderPacket으로 수집하고, InstancingManager가 DX11 버퍼와 Draw 호출을 책임진다.");
  const y = 2.05;
  flowNode(slide, "Scene / BlockData", 0.58, y, 1.65, C.blue, C.softBlue);
  arrow(slide, 2.32, y + 0.34, 0.55);
  flowNode(slide, "ChunkManager", 2.96, y, 1.52, C.cyan, "E0F2FE");
  arrow(slide, 4.58, y + 0.34, 0.55);
  flowNode(slide, "RenderPacket", 5.22, y, 1.48, C.green, C.softGreen);
  arrow(slide, 6.79, y + 0.34, 0.55);
  flowNode(slide, "InstancingManager", 7.43, y, 1.78, C.amber, C.softAmber);
  arrow(slide, 9.31, y + 0.34, 0.55);
  flowNode(slide, "DynamicInstancePool", 9.95, y, 1.86, C.red, C.softRed);
  arrow(slide, 11.9, y + 0.34, 0.45);
  flowNode(slide, "DX11 Draw", 12.18, y, 0.75, C.dark, "E2E8F0");
  placeholder(slide, 0.62, 3.18, 5.55, 2.55, "CODE CAPTURE: RenderPacket / InstancingManager", "핵심 코드 1~2개 캡처");
  placeholder(slide, 6.45, 3.18, 6.1, 2.55, "PIPELINE SCREENSHOT OR DEBUG HUD", "렌더링 통계가 보이는 화면");
  body(slide, [
    "설계 포인트",
    "Scene은 렌더링 API를 직접 알지 않음",
    "Instance 데이터는 DynamicInstancePool/Ring Buffer를 통해 갱신",
    "Constant Buffer와 Instance Buffer 책임을 분리",
  ], 0.7, 6.15, 4.6, 0.88, { fontSize: 8.8, bullet: true });
  body(slide, [
    "포폴 문장",
    "렌더링 후보 선별, 인스턴스 버퍼 업로드, 실제 Draw 호출을 계층별로 분리해 병목 위치를 추적 가능하게 구성했습니다.",
  ], 6.65, 6.15, 5.65, 0.82, { fontSize: 8.8 });
}

function slide03() {
  const slide = pptx.addSlide();
  addBg(slide);
  addTop(slide, "OPTIMIZATION", 3);
  title(slide, "DrawCall과 렌더 후보를 줄이기 위한 네 가지 최적화", "최적화 자체보다 중요한 것은, 각 기능을 끄고 켤 수 있게 만들어 같은 조건에서 효과를 검증한 점이다.");
  const cards = [
    ["Hardware Instancing", "문제", "블록 개수만큼 Draw 호출이 증가", "해결", "동일 Mesh/Material을 그룹화해 DrawIndexedInstanced 호출", C.blue],
    ["Frustum Culling", "문제", "화면 밖 Entity도 렌더 후보에 포함", "해결", "Camera Frustum 기준 Visible/Culled 수집", C.cyan],
    ["Face Occlusion", "문제", "완전히 내부에 묻힌 블록까지 후보 유지", "해결", "6방향 이웃이 모두 막힌 블록 제외", C.green],
    ["SmartRebuild", "문제", "수정 없는 그룹까지 매 프레임 재빌드", "해결", "Dirty MeshGroup만 재빌드하고 Skip 수집", C.amber],
  ];
  cards.forEach((c, i) => {
    const x = 0.55 + i * 3.1;
    slide.addShape(pptx.ShapeType.rect, {
      x,
      y: 1.95,
      w: 2.78,
      h: 2.3,
      fill: { color: "FFFFFF" },
      line: { color: "E2E8F0", width: 0.6 },
    });
    slide.addShape(pptx.ShapeType.rect, {
      x,
      y: 1.95,
      w: 2.78,
      h: 0.08,
      fill: { color: c[5] },
      line: { color: c[5] },
    });
    slide.addText(c[0], { x: x + 0.14, y: 2.15, w: 2.5, h: 0.22, fontSize: 10.2, bold: true, color: C.ink, margin: 0, fit: "shrink" });
    label(slide, c[1], x + 0.14, 2.58, 0.55, C.red);
    body(slide, [c[2]], x + 0.14, 2.82, 2.46, 0.36, { fontSize: 7.8, color: C.ink });
    label(slide, c[3], x + 0.14, 3.38, 0.55, C.green);
    body(slide, [c[4]], x + 0.14, 3.62, 2.46, 0.42, { fontSize: 7.8, color: C.ink });
  });
  placeholder(slide, 0.62, 4.75, 5.9, 1.78, "BENCHMARK HUD SCREENSHOT", "최적화 토글 ON/OFF가 보이는 화면");
  table(slide, [
    ["기능", "측정 지표", "포폴에서 말할 근거"],
    ["Instancing", "DrawCalls / Instances", "개별 렌더링이 아니라 GPU Instancing 사용"],
    ["Frustum", "Visible / Culled", "화면 밖 후보 제거"],
    ["Face", "Face Culled / Instances", "블록 구조 특화 후보 제거"],
    ["Smart", "Rebuild / Skip", "불필요한 재빌드 방지"],
  ], 6.8, 4.73, [1.15, 1.65, 3.1], 0.34);
}

function slide04() {
  const slide = pptx.addSlide();
  addBg(slide);
  addTop(slide, "CONTROLLED BENCHMARK", 4);
  title(slide, "Before/After가 아니라 같은 씬의 Baseline/Optimized 비교", "과거 커밋을 억지로 재현하지 않고, 최적화 옵션을 끈 기준 상태와 현재 구현을 같은 조건에서 비교한다.");
  placeholder(slide, 0.55, 1.86, 3.9, 2.05, "FRUSTUM HIGH", "ON/OFF 비교 캡처");
  placeholder(slide, 4.72, 1.86, 3.9, 2.05, "FACE HIGH", "ON/OFF 비교 캡처");
  placeholder(slide, 8.89, 1.86, 3.9, 2.05, "SMART HIGH", "ON/OFF 비교 캡처");
  table(slide, [
    ["Scene", "Mode", "Blocks", "Visible", "Culled", "DrawCalls", "Instances", "CPU"],
    ["Frustum High", "Baseline", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기"],
    ["Frustum High", "Optimized", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기"],
    ["Face High", "Baseline", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기"],
    ["Face High", "Optimized", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기"],
    ["Smart High", "Baseline", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기"],
    ["Smart High", "Optimized", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기", "붙여넣기"],
  ], 0.56, 4.38, [1.42, 1.05, 1.0, 1.0, 1.0, 1.08, 1.1, 0.9], 0.35);
  body(slide, [
    "캡처 절차: 시나리오 선택 -> Camera -> Baseline -> 옵션 변경 -> Optimized -> HUD 비교표 캡처",
    "보고 문장: 동일 씬에서 옵션만 제어한 controlled benchmark로 최적화 효과를 검증했습니다.",
  ], 0.7, 6.98, 11.9, 0.32, { fontSize: 7.8, color: C.muted });
}

function slide05() {
  const slide = pptx.addSlide();
  addBg(slide);
  addTop(slide, "RESULT / TAKEAWAY", 5);
  title(slide, "렌더링 이해를 증명하는 엔진형 포트폴리오", "3개월 동안 구현한 핵심 가치는 게임 콘텐츠보다 렌더링 병목을 직접 다루고 검증 가능한 구조로 만든 데 있다.");
  placeholder(slide, 0.58, 1.86, 5.05, 3.15, "FINAL COMPARISON SCREENSHOT", "Baseline vs Optimized 표가 보이는 HUD");
  body(slide, [
    "핵심 성과",
    "DirectX11 Buffer / Shader / Draw 호출 흐름 직접 제어",
    "대량 블록 데이터를 Chunk 단위로 관리",
    "Hardware Instancing으로 DrawCall을 구조적으로 절감",
    "Frustum / Face / SmartRebuild를 옵션화해 검증 가능하게 구성",
    "Editor HUD, CSV, Dump로 포폴 근거 자료를 추출 가능",
  ], 6.05, 1.88, 3.42, 2.15, { bullet: true, fontSize: 9.1 });
  body(slide, [
    "면접 설명 포인트",
    "이 프로젝트는 상용 게임보다 렌더링 엔진 이해를 보여주는 테크 데모입니다.",
    "최적화 기능을 구현한 뒤, 같은 씬에서 ON/OFF 비교가 가능한 검증 도구까지 만들었습니다.",
  ], 9.75, 1.88, 2.85, 2.08, { fontSize: 9.1 });
  table(slide, [
    ["역량", "근거"],
    ["DX11 Low-Level", "Buffer / Shader / DrawIndexedInstanced"],
    ["CPU-GPU 병목 이해", "DrawCall, Instance, Rebuild 비용 측정"],
    ["엔진 구조 설계", "Scene / RHI / UI 디커플링"],
    ["검증 태도", "Controlled Benchmark HUD 구현"],
  ], 6.05, 4.38, [1.65, 4.85], 0.42);
  body(slide, [
    "마지막 체크: 실제 포폴에는 수치가 들어간 HUD 캡처 3장과 구조도 1장을 붙이면 완성됩니다.",
  ], 0.65, 5.45, 4.8, 0.44, { fontSize: 8.4, color: C.muted });
}

[slide01, slide02, slide03, slide04, slide05].forEach((fn) => fn());

await pptx.writeFile({ fileName: outFile });
console.log(outFile);
