(() => {
  const example = document.querySelector(".quill-example");
  if (!example) return;

  const run = example.querySelector(".quill-example-run");
  const link = example.querySelector(".quill-example-open");
  const preview = example.querySelector(".quill-example-preview");
  run.hidden = false;
  run.addEventListener("click", () => {
    const url = new URL(link.href);
    url.pathname = "/e";
    const iframe = document.createElement("iframe");
    iframe.title = "Edit and run the Quill C++ example in Compiler Explorer";
    iframe.referrerPolicy = "no-referrer";
    iframe.src = url.href;
    preview.replaceChildren(iframe);
    run.hidden = true;
    iframe.focus();
  }, { once: true });
})();
