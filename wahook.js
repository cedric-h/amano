class Shader {
  constructor(program) {
    this.program = program
  }
}

class Buffer {
  constructor(ib, vb) {
    this.ib = ib
    this.vb = vb
  }
}

class Renderer {
  constructor(gl) {
    this.gl = gl
    this.gl_ex_lose_context = gl.getExtension("WEBGL_lose_context")

    if (!gl) {
      console.log('falling back to experimental webgl');
      gl = canvas.getContext('expreimental-webgl');
    }

    gl.enable(gl.DEPTH_TEST);
    gl.enable(gl.CULL_FACE);
    gl.frontFace(gl.CCW);
    gl.cullFace(gl.BACK);

    canvas.addEventListener('webglcontextlost', (evnt) => {
      console.log("WebGL context lost, restoring");
      this.gl_ex_lose_context.restoreContext();
    });
  }

  createShader(vertexText, fragmentText) {
    var vertexShader = this.gl.createShader(this.gl.VERTEX_SHADER);
    var fragmentShader = this.gl.createShader(this.gl.FRAGMENT_SHADER);
    
    this.gl.shaderSource(vertexShader, vertexText);
    this.gl.shaderSource(fragmentShader, fragmentText);
    
    this.gl.compileShader(vertexShader);
    if (!this.gl.getShaderParameter(vertexShader, this.gl.COMPILE_STATUS))
    {
      console.error("failed compiling vertex shader", this.gl.getShaderInfoLog(vertexShader));
      return null;
    }
      
    this.gl.compileShader(fragmentShader);
    if (!this.gl.getShaderParameter(fragmentShader, this.gl.COMPILE_STATUS))
    {
      console.error("failed compiling vertex shader", this.gl.getShaderInfoLog(fragmentShader));
      return null;
    }

    var program = this.gl.createProgram();
    this.gl.attachShader(program, vertexShader);
    this.gl.attachShader(program, fragmentShader);
    
    this.gl.linkProgram(program);
    if (!this.gl.getProgramParameter(program, this.gl.LINK_STATUS))
    {
      console.error('error linking program', this.gl.LINK_STATUS);
      return null;
    }

    this.gl.validateProgram(program);
    if (!this.gl.getProgramParameter(program, this.gl.VALIDATE_STATUS))
    {
      console.error('error validating program', this.gl.VALIDATE_STATUS);
      return null;
    }

    return new Shader(program);
  }

  updateBuffer(buffer, vertices, indices) {
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, buffer.vb)
    this.gl.bufferData(this.gl.ARRAY_BUFFER, vertices, this.gl.DYNAMIC_DRAW)

    this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, buffer.ib)
    this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, indices, this.gl.DYNAMIC_DRAW)
  }

  createBuffer(vertices, indices) {
    let buf = new Buffer(this.gl.createBuffer(), this.gl.createBuffer())
    this.updateBuffer(buf, vertices, indices)
    return buf
  }
}
