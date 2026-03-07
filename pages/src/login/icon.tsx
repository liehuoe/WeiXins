export function Icon(props: {
  icon: string;
  class?: string;
  title?: string;
  onClick?: (e: MouseEvent) => void;
  ref?: (el: HTMLElement) => void;
}) {
  return (
    <button
      class={`border-none cursor-pointer border-rd-2 p-1 shadow-dark shadow-lg hover:bg-op-70 ${props.class}`}
      title={props.title}
      onclick={props.onClick}
      ref={props.ref}
    >
      <div class={`border-rd-2 text-xl ${props.icon}`} />
    </button>
  );
}
